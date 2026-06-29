# KRSH
KRSH is a very basic, but customizable shell, for now only supports linux,  
and because it is basic, it is super fast and minimalist  
but for whatever reason if you want me to make it support windows, just let me know in the pull requests.  
(also I am sorry for those of you who want to read the source code I didn't put any comments, but will later)  
and for those who want to compile it, I have included the makefile, but if you don't have make, run:  
`pacman -S make`  
or  
`apt install make`  
and then inside the KRSH directory, type the `make` command.
 
# The Plugin System
KRSH has a built in plugin system that requires some c++ knowledge but other than that very simple.  
To make a plugin, you will need to compile it using either the pluginCompiler.sh file or type `g++ -O3 -shared -fPIC plugin.cpp -o plugin.o` but replace the plugin part to your source and output file then put it inside ~/.config/krsh/plugins

# Plans
I am planning to add much more to it, but for now it's pretty decent,  
but that's where **YOU** come in!  
I want *you* to point out bugs and leave suggestions **to make KRSH better!**

