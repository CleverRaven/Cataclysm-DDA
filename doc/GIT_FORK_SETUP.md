
# Local Git setup

This guide is intended for beginner and intermediate git users on how to set up git locally to make submitting or testing
pull requests easily with less clutter and minimizing issues (like having an outdated local copy). It may also have a tip
or two for advanced suers.

## Installing the requirments

Git is a terminal tool. Yes UI tools exist, but they can be traps because even using them, it will be important to understand
what git is doing under the hood. This guide will only talk about how to use a cli git. It will give you all the commands you
need, so no need to be afraid.

Depending on your OS, install or ensure you have already git: `git --version` in your terminal. OSX or linux will most likely
already have it, but for windows users I suggest grabbing a "Git Bash" rather than using the default Command Prompt.

## Github.com

You will need a Github account. Use [Github tutorial](https://help.github.com/en/articles/connecting-to-github-with-ssh)  on how to set up an "ssh key", this will authenticate you.

### Prep your fork

You do not want a `master` branch in your fork (i.e. your personal copy). You will never commit to it, it is not useful and
keeping it constantly up to date is a chore. So lets look at how to get rid of it. Github requires there to be a default
branch, but we have no need of it, so lets create a placeholder branch to be your default. 

1. Go to cata repo and click "fork".
2. Click the `Branch: master \/` button and write `placeholder` in the textbox.
3. Go to the `Settings` tab, and remove checkboxes for `Wikis`, `Projects`
4. Then select the `Branches` pane on the `Settings` tab and on the dropdown that says `master \/`, choose `placeholder`
5. Accept the warning

This placeholder branch will grow out of date quickly, but it doesn't matter. Basically never think about it again.

## Downloading the code

Go to your fork (1.2.3) and copy the ssh address of your repo, something like git@git. Open your bash/terminal and find the
folder where you want the code to be and type either:

 - `git clone <git address> .` (if you are in your cataclysm folder)
 - `git clone <git address> cdda` (if you havent made a dedicated folder yet for where to have the game files, it will make the `cdda` folder)

You know have the code from your fork locally. It is time to do some cleanup of more things that you wont need. First lets get
ridd of your `master` completely (to avoid any confusion):

 - `git push origin :master` (deletes the master branch from your Github fork).

### [Optional] More cleanup tags

This step is optional, but recommended and something I do because it makes things even more clean and clear. Let's remove all the tags
and branches that exist in the original game repo.

 - `git push origin --delete $(git tag -l)` (Removes all tags from your Github fork)
 - `git tag -l | xargs git tag -d` (Removes all tags locally)
 - Call to `git tag` should no display nothing.

If there is a problem with these scripts that does all in one go, the commands to delete tags manaully is:
`git push origin :tag-name` and `git tag -d tagname`.

### [Optional] More cleanup branches

If you do `git branch --all` now, you should see that you have one local branch called `placeholder` and that your remote have a lot of
branches for previous releases and current features that Kevin is working on. These aren't useful for you, so it may be nice to remove them.
You want to end up with something like this:

```
* placeholder
  remotes/origin/HEAD -> origin/placeholder
  remotes/origin/placeholder
```

To do this use this command:

 - `git branch --no-color -r --list 'origin/*' | grep -v placeholder | sed 's/origin\///' | xargs git push --delete origin` (removes all except placeholder)

If this command throws and error or doesnt produce the result above when you repeat `git branch --all`, you can try this alternative more manual way:

 - `git push origin -d master 0.A-branch 0.B-branch 0.C-branch 0.D-branch 0.9-branch 0.8-branch gh-pages` (Removes known branchs)
 - `git branch -r | grep -v placeholder | cut -d/ -f2-` (Lists all the names of branches we dont need)
 - You can delete the branches manually with `git push origin -d <branch-name>`, for example `git push origin -d gh-pages`. 

## Add the "UPSTREAM"

So the word "origin" you see all over these commands refers to your copy (the fork). This is where you will upload (push) changes. But to get changes
down from the original game repository (which we will call "upstream"), you need to add a second source (called "remote").

To do this, call this command:

 - `git remote add upstream -t master --no-tags  git@github.com:CleverRaven/Cataclysm-DDA.git` (Adds the upstream remote)
 - `git fetch upstream` (download any changes in remote)
 - `git checkout -b master upstream/master` (create a local master branch that tracks the upstream master)
 - `git branch -d placeholder` (removes the placeholder locally since you need never check it out or worry about it again)

If you do `git branch --all` you should now see this:
```
* master
  remotes/origin/HEAD -> origin/placeholder
  remotes/origin/placeholder
  remotes/upstream/master
```

## Viola!

So now you are well set up. Your fork is clean and you have a master locally that you can keep up to date with the games main repo:

 - `git checkout master` (Makes sure you are "in" the local master, for example after having worked on a feature)
 - `git pull origin master` (Downloads changes in the main repo and merges it into your local master).

## How to make a Pull Request

So you have seen an [issue with the "Good First Issue" tag](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Good+First+Issue%22) and now you want to make a go at it. Here is what you do:

 - `git checkout master` (Makes sure you are "in" the local master, for example after having worked on a feature)
 - `git pull origin master` (Downloads changes in the main repo and merges it into your local master).
 - `git checkout -b <branch-name>` (with `<branch-name>` being replaced with something describing your change, short and with letters and `-`, for example `git checkout -b feature-flight`)
 - Edit files and have fun.
 - Compile and test. Look at [how to compile](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/COMPILING.md) for help on that.
 - `git add -p` (asks you for each change it discovers in your local files if it should be included)
 - `git commit -m "<a description of this changeset>"` (Makse the commit message describing the issue)
 - `git push origin <branch-name>` (Pushes your commits to Github, to your fork. replace `<branch-name>` ofc.)

Now you can go to [https://github.com/CleverRaven/Cataclysm-DDA](https://github.com/CleverRaven/Cataclysm-DDA) and it 
should detect your new branch in a yellow notice there and ask you if you want to create a PR. There is also a a "new pull
request" button on your github fork page.

You will probably get some feedback on your Pull request. The first thing to remember is that this is not critisism, this is 
people helping you out, making sure that your PR will work well and be of good quality. They are your ad hoc team! Sometimes
you can accept changes they suggest directly in the web interface, but other times you want to go back to your local files.
This is no problem, you can make more edits and more commits and push again (last 3 steps above). Until your PR is accepted.

After your PR is accepted, you can delete this branch and get ready for more. If you delete the branch using the button on the PR, skip the 3rd step.

 - `git checkout master`
 - `git branch -d <branch-name>`
 - `git push origin -d <branc-name>`
 - `git pull origin master`

Now you are ready to start from the step above where you create your next feature branch.

Hope this was helpful. Good luck!
