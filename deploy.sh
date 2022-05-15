BRANCH_NAME="main"
MAKEFILE_NAME="makefile.unix"

if [ -z "$1" ]; 
then echo "No branch name provided, using \"$BRANCH_NAME\""
else BRANCH_NAME=$1
fi

if [ "$1" != "make"]
# Fetching remote changes
git fetch
git checkout $BRANCH_NAME
git pull
fi

# Compiling
make -f $MAKEFILE_NAME
make -f $MAKEFILE_NAME clean

echo "Compiled at $EW_HOME/$EW_VERSION/bin/"
