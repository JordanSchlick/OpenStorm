mkdir -p tmp
rsync -a -m --info=progress2 --delete --delete-excluded --include-from=include.txt ../../ ./tmp/source/