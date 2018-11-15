# A script to test the generation of a stub for one C file
# ./testFile [-r] filename
# It compiles the source
# Generates a stub for the C file (should be in $dir/mains/)
# Draws its Dependence Graphs as a pdf
# Compiles the stubs
# Execute it
#
# Option:
#  -r : Delete the .dot and .ctr files produced
#

echo -e "\033[34m[Starting Compilation]\033[0m"
dir="stubTests"

make || { echo -e "\033[33m Compilation failed \033[0m"; exit 1; }
echo -e "\033[34m[Starting Test]\033[0m"

if [[ $1 -eq "-r" ]]; then
	f=$(basename $2 .c)
else
	f=$(basename $1 .c)
fi
mkdir $dir/mains 2> /dev/null 
echo "Testing file $f.c."
time ./Gen $dir/$f.c clean 
if [[ $? -eq 139 ]]; then
	echo -e "\033[33m A segmentation fault occurs with file $dir/$f.c\033[0m"
fi

echo -e "\033[34m[Drawing graphs]\033[0m"
mkdir $dir/graphs 2> /dev/null 
for d in $dir/$f*.depGraph.dot; do
        pdfName=$(basename $d .depGraph.dot)
        pdfName="$dir/graphs/$pdfName.pdf"
        dot -Tpdf $d -o $pdfName 2> /dev/null
done

echo -e "\033[34m[Cleaning old exec]\033[0m"
rm $dir/mains/$f*.out 2> /dev/null

echo -e "\033[34m[Compiling test]\033[0m"
for s in $dir/mains/${f}_*_main.c; do
	execName=$(basename $s _main.c)
	execName=$dir/mains/$execName.out
	gcc -g -Wall $s -o $execName && echo "Successful compilation of $s into $execName"
done

echo -e "\033[34m[Executing tests]\033[0m"
mkdir $dir/csv 2> /dev/null 
cd $dir/mains/
for e in ${f}_*.out; do
	echo "./$e"
	./$e && echo "Successful execution of $e"
done
cd ../../

echo -e "\033[34m[Cleaning options]\033[0m"
while getopts ":r" opt ; do
	case $opt in
	 r)
		echo "Clearing *.dot and *.ctr"
		rm $dir/$f*.dot
		rm $dir/$f*.ctr
		;;
	 \?)
		echo "Invalide option: -$opt"
		;;
	esac
done
echo -e "\033[34m[Script end]\033[0m"
