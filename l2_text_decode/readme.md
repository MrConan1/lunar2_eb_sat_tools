This is a dialog text dumper for L2 Eternal Blue on Sega Saturn & PSX.  
Common battle/item text decoding is not supported, the format for that is different.  
Input are "ES" script files (Header of 0x45530001)  

Output:
  Saturn Files will output in utf-8 format.  
  JP PSX Files will output in sjis format.  
  US PSX Files will output in standard ascii.  


Usage:  
l2_txt_decode.exe InputFname type offset  
type = 0 for jp saturn, type = 1 for jp psx, 2 for us psx  

OR:  Copy the script files to the appropriate folder and run the corresponding batch file.  
