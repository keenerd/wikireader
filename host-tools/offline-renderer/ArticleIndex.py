#! /usr/bin/env python
# -*- coding: utf-8 -*-
# COPYRIGHT: Openmoko Inc. 2009
# LICENSE: GPL Version 3 or later
# DESCRIPTION: Simple FLASH programmer
# AUTHORS: Sean Moss-Pultz <sean@openmoko.com>
#          Christopher Hall <hsw@openmoko.com>

import os, sys, re
import struct
import littleparser
import getopt
import os.path
import cPickle
import FilterWords
import FileScanner


tparser = littleparser.LittleParser() # for handling titles

# this _must_ be in ascending ASCII sequence
KEYPAD_KEYS = """ !#$%&'()*+,-.0123456789=?@abcdefghijklmnopqrstuvwxyz"""

# to check if in order: uncomment and look at result
#for c in KEYPAD_KEYS:
#    print ord(c)


# redirect: <text.....#redirect.....[[title#relative link]].....
redirected_to = re.compile(r'#redirect[^\[]*\[\[(.*?)([#|].*?)?\]\]', re.IGNORECASE)

# Filter out Wikipedia's non article namespaces
non_articles = re.compile(r'User:|Wikipedia:|File:|MediaWiki:|T(emplate)?:|Help:|Cat(egory)?:|P(ortal)?:', re.IGNORECASE)

# underscore and space
whitespaces = re.compile(r'([\s_]+)', re.IGNORECASE)


verbose = False
bigram = {}


def usage(message):
    if None != message:
        print 'error:', message
    print 'usage: %s <options> {xlm-file...}' % os.path.basename(__file__)
    print '       --help                  This message'
    print '       --verbose               Enable verbose output'
    print '       --article-index=file    Article index dictionary output [articles.pickle]'
    print '       --article-offsets=file  Article file offsets dictionary output [offsets.pickle]'
    print '       --article-counts=file   File to store the counts [counts.text]'
    print '       --prefix=name           Device file name portion for .fnd/.pfx [pedia]'
    exit(1)


def main():
    global verbose
    global redirects, article_index

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hvi:o:c:p:',
                                   ['help', 'verbose',
                                    'article-index=',
                                    'article-offsets=',
                                    'article-counts=',
                                    'prefix='])
    except getopt.GetoptError, err:
        usage(err)

    verbose = False
    art_name = "articles.pickle"
    off_name = "offsets.pickle"
    cnt_name = "counts.text"
    fnd_name = 'pedia.fnd'
    pfx_name = 'pedia.pfx'

    for opt, arg in opts:
        if opt in ('-v', '--verbose'):
            verbose = True
        elif opt in ('-h', '--help'):
            usage(None)
        elif opt in ('-i', '--article-index'):
            art_name = arg
        elif opt in ('-o', '--article-offsets'):
            off_name = arg
        elif opt in ('-c', '--article-counts'):
            cnt_name = arg
        elif opt in ('-p', '--prefix'):
            fnd_name = arg + '.fnd'
            pfx_name = arg + '.pfx'
        else:
            usage('unhandled option: ' + opt)


    processor = FileProcessing()
    for f in args:
        processor.process(f)

    # record initial counts
    a = processor.article_count
    r = processor.redirect_count
    a2 = len(processor.article_index)

    # fix up redirects
    processor.resolve_redirects()

    # record combined count and display statistics
    m = len(processor.article_index)
    s = a + r

    cf = open(cnt_name, 'w')

    for f in (sys.stdout, cf):
        f.write('Articles:   %10d\n' % a)
        f.write('Articles2:  %10d\n' % a2)
        f.write('Redirects:  %10d\n' % r)
        f.write('Sum:        %10d\n' % s)
        f.write('Merged:     %10d\n' % m)
        f.write('Difference: %10d\n' % (m - s))

        f.write('Restricted: %10d\n' % processor.restricted_count)

    cf.close()

    output_fnd(fnd_name, processor.article_index)
    output_pfx(pfx_name)
    output_index(art_name, processor.article_index)
    output_offsets(off_name, processor.article_offsets, processor.all_file_names())




def generate_bigram(text):
    global bigram
    if len(text) > 2:
        try:
            if ord(text[0]) < 128 and ord(text[1]) < 128:
                bigram[text[0:2]] += 1
        except KeyError:
            bigram[text[0:2]] = 1

    if len(text) > 4:
        try:
            if ord(text[2]) < 128 and ord(text[3]) < 128:
                bigram[text[2:4]] += 1
        except KeyError:
            bigram[text[2:4]] = 1


class FileProcessing(FileScanner.FileScanner):

    def __init__(self, *args, **kw):
        super(FileProcessing, self).__init__(*args, **kw)

        self.restricted_count = 0
        self.redirect_count = 0
        self.article_count = 0
        self.article_index = {}   # array: [article_number, fnd_index]
        self.article_offsets = [None] # tuple: (file_id, title, seek, length)
        self.translate = littleparser.LittleParser().translate
        self.redirects = {}


    def title(self, text, seek):
        global non_articles

        if non_articles.search(text):
            if verbose:
                print 'Non-article Title:', text
            return False

        return True


    def redirect(self, title, text, seek):
        global non_articles, redirected_to, whitespaces
        match = redirected_to.search(text)
        if match:
            redirect_title = self.translate(match.group(1)).strip().strip(u'\u200e\u200f')
            redirect_title = whitespaces.sub(' ', redirect_title).strip()
            if non_articles.search(text):
                if verbose:
                    print 'Non-article Redirect:', text
                return

            self.redirects[title] = redirect_title
            self.redirect_count += 1
            if verbose:
                print 'Redirect: %s -> %s' % (title.encode('utf-8'), redirect_title.encode('utf-8'))
        else:
            print 'Invalid Redirect: %s -> %s' % (title.encode('utf-8'), text.encode('utf-8'))


    def body(self, title, text, seek):

        title = self.translate(title).strip(u'\u200e\u200f')

        restricted = not filter(title) or not filter(text)

        self.article_count += 1
        if restricted:
            self.restricted_count += 1

        if not verbose and self.article_count % 10000 == 0:
            print self.article_count

        generate_bigram(title)

        if verbose:
            if restricted:
                print 'Restricted Title:', title.encode('utf-8')
            else:
                print 'Title:', title.encode('utf-8')

        self.article_offsets.append((self.file_id(), title, seek, len(text)))

        if title in self.article_index:
            print 'Duplicate Title:', title.encode('utf-8')

        self.article_index[title] = [self.article_count, -1, restricted] # -1 == pfx place holder


    def resolve_redirects(self):
        """add redirect to article_index"""
        for item in self.redirects:
            try:
                self.article_index[item] = self.find(item)
            except KeyError:
                print 'Unresolved redirect:', item, '->', self.redirects[item]


    def find(self, title):
        """get index from article title

        also handles redirects
        returns: [index, fnd]
        """
        try:
            title = self.redirects[title]
        except KeyError:
            title = self.redirects[title[0].swapcase() + title[1:]]

        try:
            result = self.article_index[title]
        except KeyError:
            try:
                result = self.article_index[title[0].swapcase() + title[1:]]
            except:
                result = self.find(title)
        return result



non_letters = re.compile('[\d\W]+')
max_score = 1

def filter(text):
    """check if text contains any restricted words"""
    global non_letters, max_score
    score = 0
    contains = {}
    for w in non_letters.split(text):
        word = w.lower()
        if word in FilterWords.filter_words:
            score += FilterWords.filter_words[word]
            if word in contains:
                contains[word] += 1
            else:
                contains[word] = 1

    return score < max_score


import unicodedata
def strip_accents(s):
    if type(s) == str:
        s = unicode(s, 'utf-8')
    return ''.join((c for c in unicodedata.normalize('NFD', s) if unicodedata.category(c) != 'Mn'))


def bigram_encode(title):
    global bigram

    result = ''
    title = strip_accents(title)

    while len(title) >= 2:
        if ord(title[0]) < 128:
            b = title[0:2]
            if b in bigram:
                result += bigram[b]
                title = title[2:]
            else:
                result += chr(ord(title[0:1]))
                title = title[1:]
        else:
            #result += '?'
            title = title[1:]
    if len(title) == 1:
        if ord(title[0]) < 128:
            result += chr(ord(title[0]))
        #else:
        #    result += '?'
    return result


def output_fnd(filename, article_index):
    """create bigram table"""
    global bigram
    global index_matrix

    print 'Writing:', filename
    out_f = open(filename, 'w')

    sortedgram = [ (value, key) for key, value in bigram.iteritems() ]
    sortedgram.sort()
    sortedgram.reverse()

    bigram = {}
    i = 0
    for k, v in sortedgram:
        out_f.write(v)
        bigram[v] = chr(i + 128)
        i += 1
        if i >= 128:
            break
    while i < 128:
        out_f.write('zz')
        bigram['zz'] = chr(i + 128)
        i += 1

    # create pfx matrix and write encoded titles
    article_list = article_index.keys()
    #article_list = [strip_accents(k) for k in article_index.keys()]

    #article_list.sort(key = unicode.lower)
    article_list.sort(key = lambda x: strip_accents(x).lower())
    index_matrix = {}
    index_matrix['\0\0\0'] = out_f.tell()
    for title in article_list:
        offset = out_f.tell()
        key3 = (title[0:3] + '\0\0\0')[0:3].lower()
        key2 = key3[0:2] + '\0'
        key1 = key3[0:1] + '\0\0'
        if key1 not in index_matrix:
            index_matrix[key1] = offset
        if key2 not in index_matrix:
            index_matrix[key2] = offset
        if key3 not in index_matrix:
            index_matrix[key3] = offset
        article_index[title][1] = offset
        out_f.write(struct.pack('Ib', article_index[title][0], 0) + bigram_encode(title) + '\0')

    out_f.close()


def output_pfx(filename):
    """output the pfx matrix"""
    global index_matrix

    print 'Writing:', filename
    out_f = open(filename, 'w')
    list = '\0' + KEYPAD_KEYS
    for k1 in list:
        for k2 in list:
            for k3 in list:
                key = k1+k2+k3
                if key in index_matrix:
                    offset = index_matrix[key]
                else:
                    offset = 0
                out_f.write(struct.pack('I', offset))

    out_f.close()


def output_index(filename, article_index):
    """output the article data"""

    print 'Writing:', filename
    output = open(filename, 'wb')
    cPickle.dump(article_index, output)
    output.close()


def output_offsets(filename, article_offsets, all_file_names):
    """output file names and  offsets"""

    print 'Writing:', filename
    output = open(filename, 'wb')
    cPickle.dump(article_offsets, output)
    cPickle.dump(all_file_names, output)
    output.close()


# run the program
if __name__ == "__main__":
    main()