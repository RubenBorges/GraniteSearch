TO BUILD:

1) First, unzip the .tar file inside the directory: "ibm-granite30m"

2) To enable automated building and cleaning of the program, enter the following commands from the project root:
                                                                                                                  chmod -x auto.sh
                                                                                                                  chmod -x clean.sh
                                                                                                                  chmod -x build.sh

3) From project root, execute the automated build script by entering the command: ./auto.sh 

4) The program, model and anything else required should already have been installed in the project directory "dist/"
-----------------------------------------

TO USE:

      Usage:   ./dist/GraniteSearch [path_to_gguf_model] "[query]" [file1.txt] [file2.txt] ...
      Example: ./dist/GraniteSearch ./dist/ibm-granite30m/ibm-granite "Linda dog" dist/docs/linda_the_dog.txt


