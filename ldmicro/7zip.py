import os

vers = "XXXX"
#define LDMICRO_VER_STR           "5.3.0.2"

f = open('ldversion.h','r')
for txt in f.readlines():
    if txt.find('LDMICRO_VER_STR') >= 0:
        i1 = txt.find('"')
        i2 = txt.rfind('"')
        if (i1>=0) & (i2>=0) & (i2>i1):
            vers = txt[i1+1:i2]
            vers = vers.replace('.','')
            print(vers)
            if os.path.exists('buildXXXX'):
                os.rename('buildXXXX','build'+vers)

os.system('\"C:\\Program Files\\7-Zip\\7z.exe\" a -r -tzip build'+vers+'\\build'+vers+'.zip build'+vers+'\\*.*');

os.system('pause')
#str = input("Press Enter key...")

