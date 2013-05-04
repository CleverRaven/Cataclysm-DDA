Cataclysm: Dark Days Ahead
==========================

Cataclysm: Dark Days Ahead is a roguelike set in a post-apocalyptic world. While some have described it as a "zombie game", there's far more to Cataclysm than that. Struggle to survive in a harsh, persistant, procedurally generated world. Scavenge the remnants of a dead civilization for for food, equipment, or, if you're lucky, a vehicle with a full tank of gas to get you the hell out of Dodge. Fight to defeat or escape from a wide variety of powerful monstrosities, from zombies to giant insects to killer robots and things far stranger and deadlier, and against the others like yourself, that want what you have...

## How to Compile

The current instructions on how to compile C:DDA can be found [here](http://www.wiki.cataclysmdda.com/index.php?title=How_to_compile).

## How to Contribute

Contributing to C:DDA is easy - *simply fork the repository here on GitHub, make your changes, and then send us a pull request* - but there are a couple of guidelines we suggest sticking to:

* Add this repository as an `upstream` remote.
* Keep your `master` branch clean. This means you can easily pull changes made to this repository into yours.
* Create a new branch for each new feature or set of related bug fixes.
* Never merge from your local branches into your `master` branch. Only update that by pulling from `upstream/master`.


## An Example Workflow
#### Initial setup

1. Fork this repository here on GitHub.

2. Clone your fork locally.

        git clone https://github.com/YOUR_USERNAME/Cataclysm-DDA.git
        # Clones your fork of the repository into the current directory in terminal

3. Add this repository as a remote.

        cd Cataclysm-DDA
        # Changes the active directory in the prompt to the newly cloned "Cataclysm-DDA" directory
        git remote add -f upstream https://github.com/TheDarklingWolf/Cataclysm-DDA.git
        # Assigns the original repository to a remote called "upstream"

#### Working with branches

1. For each new feature or bug fix, create a new branch.

        git branch new_feature
        # Creates a new branch called "new_feature"
        git checkout new_feature
        # Makes "new_feature" the active branch

2. Once you've committed some changes locally, you need to push them to your fork here on GitHub.

        git push origin new_feature
        # origin was automatically set to point to your fork when you cloned it
        

3. Once you're finished working on your branch and have committed and pushed all your changes, submit a pull request from your `new_feature` branch to this repository's `master` branch.

 * Note: any new commits to the `new_feature` branch on GitHub will automatically be included in the pull request, so make sure to only commit related changes to the same branch.

### Updating your `master` branch

1. Make sure you have your `master` branch checked out.

        git checkout master

2. Pull the changes from the `upstream/master` branch.

        git pull --ff-only upstream master
        # pulls changes from the "upstream" remote for the matching branch, in this case "master"