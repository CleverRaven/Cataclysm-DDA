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

#### Setup your environment

*(This only needs to be done once.)*

1. Fork this repository here on GitHub.

2. Clone your fork locally.

        $ git clone https://github.com/YOUR_USERNAME/Cataclysm-DDA.git
        # Clones your fork of the repository into the current directory in terminal

3. Add this repository as a remote.

        $ cd Cataclysm-DDA
        # Changes the active directory in the prompt to the newly cloned "Cataclysm-DDA" directory
        $ git remote add -f upstream https://github.com/TheDarklingWolf/Cataclysm-DDA.git
        # Assigns the original repository to a remote called "upstream"

#### Update your `master` branch

1. Make sure you have your `master` branch checked out.

        $ git checkout master

2. Pull the changes from the `upstream/master` branch.

        $ git pull --ff-only upstream master
        # gets changes from the "upstream" remote for the matching branch, in this case "master"

 * Note: If this gives you an error, it means you have committed directly to your local `master` branch. [Click here for instructions on how to fix this issue](#why-does-git-pull---ff-only-result-in-an-error).

#### Make your changes

0. Update your `master` branch, if you haven't already.

1. For each new feature or bug fix, create a new branch.

        $ git branch new_feature
        # Creates a new branch called "new_feature"
        $ git checkout new_feature
        # Makes "new_feature" the active branch

2. Once you've committed some changes locally, you need to push them to your fork here on GitHub.

        $ git push origin new_feature
        # origin was automatically set to point to your fork when you cloned it
        

3. Once you're finished working on your branch, and have committed and pushed all your changes, submit a pull request from your `new_feature` branch to this repository's `master` branch.

 * Note: any new commits to the `new_feature` branch on GitHub will automatically be included in the pull request, so make sure to only commit related changes to the same branch.


## Advanced Techniques

These guidelines aren't essential, but they can make keeping things in order much easier.

#### Using remote tracking branches

Remote tracking branches allow you to easily stay in touch with this repository's `master` branch, as they automatically know which remote branch to get changes from. 

    $ git branch -vv
    * master      xxxx [origin/master] ....
      new_feature xxxx ....

Here you can see we have two branches; `master` which is tracking `origin/master`, and `new_feature` which isn't tracking any branch. In practice, what this means is that git won't know where to get changes from.

    $ git checkout new_feature
    Switched to branch 'new_feature'
    $ git pull
    There is no tracking information for the current branch.
    Please specify which branch you want to merge with.

In order to easily pull changes from `upstream/master` into the `new_feature` branch, we can tell git which branch it should track. (You can even do this for your local master branch.) 

    $ git branch -u upstream/master new_feature
    Branch new_feature set up to track remote branch master from upstream.
    $ git pull
    Updating xxxx..xxxx
    ....
  
You can also set the tracking information at the same time as creating the branch.

    $ git branch new_feature_2 --track upstream/master
    Branch new_feature_2 set up to track remote branch master from upstream.

 * Note: Although this makes it easier to pull from `upstream/master`, it doesn't change anything with regards to pushing. `git push` fails because you don't have permission to push to `upstream/master`.

        $ git push
        error: The requested URL returned error: 403 while accessing https://github.com/TheDarklingWolf/Cataclysm-DDA.git
        fatal: HTTP request failed
        $ git push origin
        ....
        To https://github.com/YOUR_USERNAME/Cataclysm-DDA.git
        xxxx..xxxx  new_feature -> new_feature


## Frequently Asked Questions

####Why does `git pull --ff-only` result in an error?

If `git pull --ff-only` shows an error, it means that you've committed directly to your local `master` branch. To fix this, we create a new branch with these commits, find the point at which we diverged from `upstream/master`, and then reset `master` to that point.

    $ git pull --ff-only upstream master
    From https://github.com/TheDarklingWolf/Cataclysm-DDA
     * branch            master     -> FETCH_HEAD
    fatal: Not possible to fast-forward, aborting.
    $ git branch new_branch master          # mark the current commit with a tmp branch
    $ git merge-base master upstream/master
    cc31d0... # the last commit before we committed directly to master
    $ git reset --hard cc31d0....
    HEAD is now at cc31d0... ...

Now that `master` has been cleaned up, we can easily pull from `upstream/master`, and then continue working on `new_branch`.

    $ git pull --ff-only upstream master
    # gets changes from the "upstream" remote for the matching branch, in this case "master"
    $ git checkout new_branch