#!/usr/bin/env python
"""
 A code utilizing huffman to convert a .blib file to something that
 compresses nicely and can be uniquely decoded. This version is without
 some optimisations like bigrams.

 Copyright (C) 2008, 2009 Holger Hans Peter Freyther <zecke@openmoko.org>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

import bitwriter
import sys
import huffmanCode
import fontmap
import textrun
import codetree

# Globals
glyph_map = {}
font_map = {}
huffman_fonts = {}
huffman_glyphs = {}
huffman_x = {}
huffman_y = {}
huffman_length = {}


def determine_by_occurence(occurences):
    """Count the occurcences of something"""
    items = []
    for item in occurences.keys():
        items.append((occurences[item], item))
    items.sort(reverse=True)

    i = 0
    dict = {}
    for (count, item) in items:
        dict[item] = i
        i = i + 1

    return dict

def prepare_run(text_runs, glyph_occurences, font_occurences, x_occurences, y_occurences, length_occurences):
    """Count the occurences of fonts, glyphs and position"""

    # Sort by y position
    text_runs.sort(textrun.TextRun.cmp)

    global glyph_map, font_map
    glyph_map = determine_by_occurence(glyph_occurences)
    font_map = determine_by_occurence(font_occurences)

    # huffman foo...
    global huffman_fonts, huffman_glyphs, huffman_x, huffman_y, huffman_length
    font_input = []
    for font in font_occurences:
        font_input.append((font_occurences[font]/len(font_occurences), font))
    huffman_fonts = huffmanCode.createCodeWordMap(huffmanCode.makeHuffTree(font_input))

    glyph_input = []
    for glyph in glyph_occurences:
        glyph_input.append((glyph_occurences[glyph]/len(glyph_occurences), glyph))
    huffman_glyphs = huffmanCode.createCodeWordMap(huffmanCode.makeHuffTree(glyph_input))

    x_input = []
    for x in x_occurences:
        x_input.append((x_occurences[x]/len(x_occurences), x))
    huffman_x = huffmanCode.createCodeWordMap(huffmanCode.makeHuffTree(x_input))

    y_input = []
    for y in y_occurences:
        y_input.append((y_occurences[y]/len(y_occurences), y))
    huffman_y = huffmanCode.createCodeWordMap(huffmanCode.makeHuffTree(y_input))

    length_input = []
    for length in length_occurences:
        length_input.append((length_occurences[length]/len(length_occurences), length))
    huffman_length = huffmanCode.createCodeWordMap(huffmanCode.makeHuffTree(length_input))

    return text_runs

def write_to_file(text_runs, fonts):
    """
    A function saving the text runs and hoping autokern will do its job


    # The bitcode.....
    0    - Paragraph
    1    - Font Change

    Parapgraph:
        [0,1] - 0 no y change, 1 x and y change
        number[number] 

    Font:
        Huffman code of the font
    """

    def write_header(file):
        """
        Write out the header, the header is the code used
        for this one article
        """
        def write_foo(file, dict):
            tree = codetree.CodeTree()
            for word in dict.keys():
                tree.addCodeWord(dict[word], word)
            tree.writeTo(file)
        write_foo(file, huffman_x)
        write_foo(file, huffman_y)
        write_foo(file, huffman_length)
        write_foo(file, huffman_glyphs)

        tree = codetree.CodeTree()
        for word in huffman_fonts.keys():
            tree.addCodeWord(huffman_fonts[word], fonts[word])
        tree.writeTo(file)
    
    def write_pending_bit(writer, run):
        """
        The text run is sorted by paragrah and all glyphs of
        one paragraph are on the same line and have roughly the
        same height.
        """

        writer.write_bit(0)
        writer.write_bits(huffman_x[run.first_x])
        writer.write_bits(huffman_y[run.first_y])
 
        writer.write_bits(huffman_length[len(run.glyphs)])

        list = []
        last_glyph = None
        for glyph in run.glyphs:
            huffman_glyph = huffman_glyphs[glyph['glyph']]
            writer.write_bits(huffman_glyph)
            last_glyph = glyph

    # Code
    auto_kern_bit = open("huffmaned.cde", "w")
    write_header(auto_kern_bit)

    last_font = None
    writer = bitwriter.BitWriter()
    for text_run in text_runs:
        # we migt have a new font now
        font = text_run.font
        if last_font != font:
            writer.write_bit(1)
            writer.write_bits(huffman_fonts[text_run.font])
            last_font = font

        write_pending_bit(writer, text_run)



    bytes = writer.finish()
    auto_kern_bit.write("".join(bytes))


def usage():
    print "Wikipedia huffman coding"
    print "Usage: %s <render_text.blib> <fontmap.map>" % sys.argv[0]
    print "\t<render_text.blib> Generated by the patched GtkLauncher"
    print "\t<fontmap.map> Generated by the get_font_file.py"
    print "huffmaned.cde (Simple Wikipedia Code) is the output"
    sys.exit(1)


if len(sys.argv) < 3:
    usage()
 
# Import Psyco if available
try:
    import psyco
    psyco.full()
except ImportError:
    pass


glyphs = textrun.load(sys.argv[1])
(text_runs, glyph_occurences, font_occurences, x_occurences, y_occurences, length_occurences) = textrun.generate_text_runs(glyphs, 240)
prepare_run(text_runs, glyph_occurences, font_occurences, x_occurences, y_occurences, length_occurences)
fonts  = fontmap.load(sys.argv[2])

write_to_file(text_runs, fonts)

print "Done. Have fun!"