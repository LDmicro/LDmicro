#---------------------------------------------------------------------------
# This file is part of LDmicro.
#
# LDmicro is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LDmicro is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LDmicro.  If not, see <http://www.gnu.org/licenses/>.
#---------------------------------------------------------------------------
# PURPOSE:
#  Sorting localisation strings.
#
# USES:
#  lang-sort.py [NOSORT] | [NOADD]
#
# INPUT:
#  File 'lang.txt "created earlier using Perl scriprt" lang-make.pl.
#  File 'lang.txt' contains all string constants,
#   selected from the '* .cpp' files LDmicro project.
#
#  Files 'lang-??.txt' are created from 'lang.txt' by adding national lines
#   manually.
#  Files 'lang-??.txt' contain a pairs of localisation strings.
#  You should remain unchanged the first string of a pair.
#  You may change second string on your language.
#
#  This 'lang-sort.py' script are not changes input files
#   from the current directory.
#
# OUTPUT:
#  All sorted output files are saved in the directory 'LANG-SORT'.
#
#  When you verify the correctness of sorted files,
#   you can copy files from './LANG-SORT' directory
#   to the current working directory of the LDmicro project.
#

import os,sys
from os import listdir

if not os.path.exists('./lang-sort'):
    os.mkdir('./lang-sort')

for f in listdir('./lang-sort/'):
    if '.txt' in f:
        os.remove('./lang-sort/'+f)

doSort = 1
doAdd = 1
for a in sys.argv:
    if a.lower()=='nosort':
        doSort = 0
    if a.lower()=='noadd':
        doAdd = 0

# load the source English Dictionary from 'lang.txt', sort and save
filename = 'lang.txt'
lang_txt = []
if os.path.exists(filename):
    print('sorting: ./'+filename+' to ./lang-sort/'+filename)
    f = open('./'+filename,'rt')
    #f = open('./'+filename,'rt').read()
    #f.decode('utf8')
    for line in f.readlines():
        l = line.strip()
        if len(l) != 0:
            lang_txt.append(l)
            #print(l)
    f.close()

    # sort alphabetical
    if doSort:
        lang_txt = sorted(lang_txt)

    # writing alphabetical default lang
    lang_txt_utf8 = lang_txt
    #lang_txt_utf8.encode('utf8')
    f = open('./lang-sort/'+filename,'w')
    for l in lang_txt_utf8:
        f.write(l+'\n\n\n')
        #print(l)
    f.close()

    #os.system('pause')
    #str = input("Press Enter key...")

if doAdd == 0:
    lang_txt = []

def sortByA0(A):
    return A[0]

for filename in listdir('./'):
    if 'lang-' == filename[0:5] and '.txt' == (filename[7:]):
        # sorting translations files
        print('sorting: ./'+filename+' to ./lang-sort/'+filename)

        trans_txt = []
        trans_chenged = []
        trans_added = []

        f = open('./'+filename,'rt')
        #f = open('./'+filename,'rt').read()
        #f.decode('utf8')
        #f.close()
        i = 0

        for line in f.readlines():
            if not line:
                break
            l = line.strip()
            if len(l) == 0:
                continue
            if i == 0:
                l1 = l
                i = 1
                continue
            else:
                l2 = l
                i = 0

            a = [l1,l2]
            trans_txt.append(a)
            #print(l1+'\n')
            #print(l2+'\n\n')
        f.close()

        # test chenged string
        if len(lang_txt) > 0:
            for l1 in trans_txt:
                found = 0
                for l2 in lang_txt:
                    if l2 == l1[0]:
                        found = 1
                        break
                if found == 0:
                    aa = [l1[0],l1[1],'?']
                    #trans_chenged.append(aa) # ???

        # test added string
        for l1 in lang_txt:
            found = 0
            for l2 in trans_txt:
                if l1 == l2[0]:
                    found = 1
                    break
            if found == 0:
                a = [l1,l1]
                aa = [l1,l1, '+']
                trans_txt.append(a)
                trans_added.append(aa)

        # sorting alphabetically by the first element of the pair.
        if doSort:
            trans_txt.sort(key=sortByA0)
            trans_txt = sorted(trans_txt)

        # writing sorted
        f = open('./lang-sort/'+filename,'w')
        for l in trans_txt:
            f.write(l[0]+'\n')
            f.write(l[1]+'\n\n')
        f.close()

        # writing diffs
        if len(trans_chenged) > 0 or len(trans_added) > 0 :
            f = open('./lang-sort/'+filename[0:-4]+'-diffs'+filename[-4:],'w')
            f.write("# Strings beginning with the character '?' may be removed or changed\n");
            f.write("#  in the '*.cpp' source code of the LDmicro project.\n\n");

            trans_chenged = trans_chenged + trans_added
            if doSort:
                trans_chenged.sort(key=sortByA0)
                trans_chenged = sorted(trans_chenged)

            for l in trans_chenged:
                f.write(l[2]+' '+l[0]+'\n')
                f.write(l[2]+' '+l[1]+'\n\n')

            f.close()

#os.system('pause')
#str = input("Press Enter key...")
