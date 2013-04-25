This is the Chromium repository used by Crosswalk.

Development of Crosswalk : http://linux.intel.com/mailman/listinfo/cameo-dev

We are also on IRC : #crosswalk on otcirc.jf.intel.com:6697

## How to update this repository with upstream Chromium
* clone it
* add the upstream repo as a remote : `git remote add upstream http://git.chromium.org/chromium/src.git`
* `git checkout master`
* `git pull --rebase upstream master`
* `git push origin master`
* `git checkout -b lkgr origin/lkgr` (first time only)
* `git checkout lkgr` (other times)
* `git pull --rebase upstream lkgr`
* `git push origin lkgr`

Feel free to hack a script to automate it.

