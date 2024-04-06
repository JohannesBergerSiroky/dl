**Welcome to dl!**

This program is an experimental alternative to ls.
So far it includes file and directory listings.
It also includes user names, groups and all 
file permissions.

![An example image](dl_images/Screenshot_from_2024-03-24_14-37-50_edited.png)

![Another example image](dl_images/Screenshot_from_2024-03-24_15-48-33_edited.png)


You can build this in Linux by typing:

`gcc -Wall dl.c -o dl`

After this you can place the file in usr/local/bin for example.
Dl will list file names and directory names and truncate the length of them
if the names are bigger than 20 characters.
If you don't want that you can type:

`dl -v`

Thank you for visiting this page!
