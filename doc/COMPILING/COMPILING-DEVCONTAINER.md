# Compilation guide for devcontainers in Visual Studio Code

This guide contains instructions for compiling Cataclysm-DDA in Visual Studio Code using a [Devcontainer](https://code.visualstudio.com/learn/develop-cloud/containers)

The devcontainer was introduced in [#65748](https://github.com/CleverRaven/Cataclysm-DDA/pull/65748). These instructions were written using Visual Studio Code version 1.80.1 on Debian 11

## Prerequisites:

* Visual Studio Code
* [Docker](https://docs.docker.com/engine/install/)


## Installation:

1. Install all prerequisites.
2. Clone your fork of the CleverRaven/Cataclysm-DDA repo and create a branch
3. Add Cleverraven as the remote upstream with git remote add upstream git@github.com:CleverRaven/Cataclysm-DDA.git
4. Open the folder where you cloned your repository in Visual Studio Code via the UI or by navigating to the directory in a terminal and typing Code
5. Visual Studio Code will show a pup-up in the bottom right with recommended extentions. Install those.
6. Now restart visual studio code. When prompted, click "Reopen in container":
   
  ![Re-open devcontainer in vscode](../img/Devcontainer-Re-Open-In-Container.png)

  6.1 Linux only: You may see this message:
  
  ![User does not have access to group, add user to docker group first](../img/Devcontainer-User-Does-Not-Have-Access-Add-To-Group-First.png)
  
  In that case, add the user to the docker group using the terminal:
  
  ![Add user to docker group in terminal](../img/Devcontainer-Add-User-To-Docker-Group.png)

  After that, log out of your account and log back in so the permissions are updated. If that doesn't work, reboot your computer.

  
7. Allow the container to build and for VSCode to Reopen. If everything goes well, you will see the container running:

  ![Image showing the container is running in vscode](../img/Devcontainer-Running-Cataclysm-Devcontainer.png)


8. Select the makefile extension on the bottom left and choose your desired configuration. Press the "Play" button to build the project

  ![Image the buttons to press in the makefile extention](../img/Devcontainer-Make-File-Configs.png)


  
The build result should be located in the folder where you cloned your fork to (e.g. /workspaces/Cataclysm-DDA)

You can then test your build using this command in the terminal:
```bash
./cataclysm-tiles
```



# Cross-compiling from Linux to Windows
1. Follow all of the steps written earlier in this guide

2. Open the Dockerfile in the VSCode file browser and scroll down to find this section:

  ![Image showing the commented part of the dockerfile](../img/Devcontainer-Find-Commented-Windows-Commands-In-Dockerfile.png)

  Uncomment that part (select it and press ctrl+/ in vscode) so it looks like this:
  
  ![Image showing the uncommented part of the dockerfile](../img/Devcontainer-Uncomment-Windows-Part-In-Dockerfile.png)

3. Save the Dockerfile and re-open VSCode. Allow the Devcontainer to rebuild. This is going to take a while (45 minutes depending on the performance of your computer)

  ![Image showing prompt to rebuild the devcontainer](../img/Devcontainer-Dockerfile-Changed-Prompt-Click-Rebuild.png)

4. Go to the makefile extention and set the makefile settings as follows:

  ![Image showing the makefile settings for cross compilation](../img/Devcontainer-Makefile-Settings-Crosscompile-Windows.png)
  
  Next, press the play button to build the game. After it is build, you should see the cataclysm-tiles.exe file in the folder where you cloned your fork to (e.g. /workspaces/Cataclysm-DDA)

    
