#!/bin/bash

[[ $(cat debian/changelog) =~ \(([0-9]*.[0-9]*.[0-9]*)\) ]]
LAST_CHANGELOG_VERSION=${BASH_REMATCH[1]}
echo "Last changelog version: $LAST_CHANGELOG_VERSION"
echo -n "New version (format: X.X.X): "
read NEW_CHANGELOG_VERSION

if ! [[ $NEW_CHANGELOG_VERSION =~ ^([0-9]*.[0-9]*.[0-9]*)$ ]]; then
    echo "Version string not in the expected format. Exiting"
    exit 0;
fi

dch -v $NEW_CHANGELOG_VERSION
dch -r

git add debian/changelog
git commit -m "Update debian/changelog to version $NEW_CHANGELOG_VERSION"
git tag v$NEW_CHANGELOG_VERSION
