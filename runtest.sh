#!/bin/bash
# runtest.sh
# Author:        Agner Fog
# Date created:  2019-06-02
# Last modified: 2022-07-14
# 
# This script will compile and run a testbench for the C++ Vector Class Library
# Using a list of test cases.
#
# (c) Copyright 2019-2022 by Agner Fog. 
# GNU General Public License 3.0 or later www.gnu.org/licenses
#
###############################################################################
#
# Instructions:
# ./runtest.sh listfile.lst
#
# Format for the list file:
# -------------------------
# General lines define a function or operator to test, and various parameters, 
# separated by commas:
# 1. Test case (integer or range) indicating which function or operator to test
# 2. Vector type for parameters
# 3. Vector type for return value
# 4. Instruction set (2=SSE2, 3=SSE3, 4=SSSE3, 5=SSE4.1, 6=SSE4.2, 7=AVX, 
#    8=AVX2+FMA, 9=AVX512F, 10=AVX512BW/DQ/VL, 11=AVX512VBMI2, 12=AVX512_FP16
# 5. Function name (only for permute etc.)
# 6. List of indexes for permute, blend and gather functions. Separated by '+'
#    (will be converted to template parameters separated by ',')
#
# Ranges can be specified for the following parameters:
# 1. Test case: a range specified as values separated by spaces
# 2. Vector type: Multiple vector types separted by spaces
# 3. The type for return value cannot specify a range.
#    Make it blank to get the same as the vector type for parameters
# 4. Instruction set: a range specified as values separated by spaces
#
# Special lines:
# A line beginning with a dollar sign ($) specifies a parameter:
# $compiler= (1=Gnu, 2=Clang, 
#             3=Intel compiler for Linux, legacy, 4=Intel compiler for Linux, clang based
#            10=Microsoft compiler for Windows, 11=Intel compiler for Windows, legacy)
# $mode= (32 for 32-bit mode, 64 for 64-bit mode)
# $testbench= (name of .cpp testbench file)
# $outfile= (name of output file)
# $include= (directory of .h include files. May be relative path)
# $emulator= (path and name of Intel emulator, to be called if instruction set not supported by current CPU)
# $compilermax= (maximum instruction set supported by the compiler. skip test cases with higher instruction set)
# $seed= (an integer for initializing random number generator)
#
# Comments begin with '#'
#
# The file must end with a blank line
###################################################################################################

# Extra compiler options may be inserted here:
extraoptions=""

# This fixes some bugs in gcc with testcase 5, Vec16c, instruction set 10:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65782
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89325
# Not needed in latest releases:
# gccextraoptions="-fno-asynchronous-unwind-tables -Wno-attributes"

# Default name for compiled file
exefilename="x"

# Uncomment this line if wrong operating system is detected:
# OSTYPE="cygwin"

# Detect operating system
if [[ $OSTYPE == "cygwin" || $OSTYPE == "msys" ]] ; then
  os=1  # Windows/Cygwin/Msys-Mingw
  gccextraoptions="-fno-asynchronous-unwind-tables -Wno-attributes" # workaround for cygwin problems
  exefilename="xx.exe"  # avoid same name as ELF exe file because they can be messed up
  echo -e "Operating system is Windows\n\n"
elif [[ $OSTYPE == "linux-gnu" ]] ; then
  os=2  # Linux, including Linux on Windows (WSL, Mingw-W64)
  clangextraoptions="-lm"
  echo -e "Operating system is Linux\n\n"
else
  os=0  # other operating systems may be added here
  echo -e "Operating system is Unknown\n\n"
fi


# Read file
filename=$1
if [ ! -e $filename ] ; then
echo Error: file $filename not found
exit 99
fi

# default name of output file is same as input file with extension (.lst) changed to .txt
outfile="${filename/\.[a-z]*/.txt}"

# statistics
starttime=`date +%s`
counttests=0


# Function to compile code with the specified options and run it
compileAndRun() {
  exitcode=0
  # vector type for return
  rtype=${rtype// }
  if [ -z "$rtype" ] ; then
    rtype=$vtype
  fi
  
  # function name
  if [[ ! -z "$funcname" ]] ; then
    funcname=${funcname// /}  
    parf="-Dfuncname=$funcname"
  else
    parf=""
  fi
  
  # compiler parameters
  parameters="-Dtestcase=$testcase $parf -Dvtype=$vtype -Drtype=$rtype -DINSTRSET=$instrset -Dseed=$seed"

  # indexes for template functions
  if [[ -z "$indexes" ]] ; then
    parix=""
    inx2=""
  else
    # remove spaces, replace '+' by ','
    inx2=${indexes// /}
    inx2=${inx2//\+/,}     # this is not working if IFS=","
    parix="-Dindexes=$inx2"
  fi

  # instruction set options
  if [[ $instrset -lt 3 ]] ; then
    isetoption=-msse2
  elif [[ $instrset -eq 3 ]] ; then
    isetoption=-msse3
  elif [[ $instrset -eq 4 ]] ; then
    isetoption=-mssse3
  elif [[ $instrset -eq 5 ]] ; then
    isetoption=-msse4.1
  elif [[ $instrset -eq 6 ]] ; then
    isetoption=-msse4.2
  elif [[ $instrset -eq 7 ]] ; then
    isetoption=-mavx
  elif [[ $instrset -eq 8 ]] ; then
    isetoption="-mfma -mavx2 -mf16c "
  elif [[ $instrset -eq 9 ]] ; then
    isetoption="-mfma -mavx512f -mf16c "
  elif [[ $instrset -eq 10 ]] ; then
    isetoption="-mfma -mavx512bw -mavx512dq -mavx512vl -mf16c "
  elif [[ $instrset -eq 11 ]] ; then
    isetoption="-mfma -mavx512bw -mavx512dq -mavx512vl -mf16c -mavx512vbmi -mavx512vbmi2 "
  elif [[ $instrset -eq 12 ]] ; then
    isetoption="-mfma -mavx512bw -mavx512dq -mavx512vl -mf16c -mavx512vbmi -mavx512vbmi2 -mavx512fp16 "
  fi
  
  # call compiler
  if [ $compiler -eq 0 ] ; then
      echo "Error: Compiler not specified" >> $outfile
      exitcode=1
      return 1
      
  elif [ $compiler -eq 1 ] ; then
      # Gnu compiler
      echo "g++ $parameters $isetoption $options -I$include $testbench $parix $gccextraoptions $extraoptions"
      eval g++ $parameters $isetoption $options -I$include $testbench $gccextraoptions $parix $extraoptions
      
  elif [ $compiler -eq 2 ] ; then
      # Clang compiler
      echo "clang $parameters $options $isetoption -I$include $testbench $parix $extraoptions $clangextraoptions"
      eval clang $parameters $options $isetoption -I$include $testbench $parix $extraoptions $clangextraoptions

  elif [ $compiler -eq 3 ] ; then
      # Intel compiler for Linux, legacy
      echo "icc $parameters $options $isetoption -I$include $testbench $parix $extraoptions"
      eval icc $parameters $options $isetoption -I$include $testbench $parix $extraoptions
      
  elif [ $compiler -eq 4 ] ; then
      # Intel compiler for Linux, clang based
      echo "icpx $parameters $options $isetoption -I$include $testbench $parix $extraoptions"
      eval icpx $parameters $options $isetoption -I$include $testbench $parix $extraoptions    
    
  elif [ $compiler -eq 10 ] ; then
      # MS compiler
      # instruction set options
      if [[ $instrset -le 7 ]] ; then
        isetoption=/arch:AVX
      elif [[ $instrset -eq 8 ]] ; then
        isetoption=/arch:AVX2
      elif [[ $instrset -eq 9 ]] ; then
        isetoption=/arch:AVX512
      elif [[ $instrset -eq 10 ]] ; then
        isetoption=/arch:AVX512
      fi
      parameters="/D testcase=$testcase /D vtype=$vtype /D rtype=$rtype /D INSTRSET=$instrset /D funcname=$funcname"
      echo cl.exe "$options $parameters /I$include $testbench $isetoption /D indexes=$inx2 $extraoptions"
      eval cl.exe "$options $parameters /I$include $testbench $isetoption /D indexes=$inx2 $extraoptions"
      
  elif [ $compiler -eq 11 ] ; then
      # Intel compiler for Windows
      # instruction set options
      if [[ $instrset -lt 3 ]] ; then
        isetoption=/QxSSE2
      elif [[ $instrset -eq 3 ]] ; then
        isetoption=/QxSSE3
      elif [[ $instrset -eq 4 ]] ; then
        isetoption=/QxSSSE3
      elif [[ $instrset -eq 5 ]] ; then
        isetoption=/QxSSE4.1
      elif [[ $instrset -eq 6 ]] ; then
        isetoption=/QxSSE4.2
      elif [[ $instrset -eq 7 ]] ; then
        isetoption=/QxAVX
      elif [[ $instrset -eq 8 ]] ; then
        isetoption="/QxCORE-AVX2"
      elif [[ $instrset -eq 9 ]] ; then
        isetoption="/QxCOMMON-AVX512"
      elif [[ $instrset -eq 10 ]] ; then
        isetoption="/QxCORE-AVX512"
      fi      
      parameters="/D testcase=$testcase /D vtype=$vtype /D rtype=$rtype /D INSTRSET=$instrset /D funcname=$funcname"
      echo "icl $parameters $options -I$include $testbench $isetoption $parix $extraoptions"
      eval icl.exe $parameters $options -I$include $testbench $isetoption $parix $extraoptions
      
  else
      echo "Error: Unknown compiler" >> $outfile
      exitcode=1
      return 1
  fi

  # test if compilation successful
  if [ $? -ne 0 ] ; then 
    echo "*** Compiling failed\n" >> $outfile
    exit 2    # exit
  fi
  
  # check if instruction set is supported
  if [[ $instrset -gt $maxiset ]] ; then
    if [ -e "$emulator" ] ; then
      # emulate and run compiled program
      $emulator -future -- ./$exefilename
    else
      echo "*** emulator $emulator not found\n"
      echo "*** emulator $emulator not found\n" >> $outfile
      exit 2    # exit    
    fi
  else
    # run compiled program
    ./$exefilename  
  fi
  
  returncode=$?

  if [[ $returncode -ne 0 ]] ; then 
    echo "*** Test failed\n" >> $outfile
    exit 1    # exit
  fi
}


# Function to set the compiler
setCompiler() {
  compiler=$1  # set compiler variable
  if [ $compiler -eq 1 ] ; then
    # Gnu compiler
    options="-m$mode -std=c++17 -O3 -fno-trapping-math -o$exefilename"
	# -ffinite-math-only will spoil checks for nan and infinite
  elif [ $compiler -eq 2 ] ; then
    # Clang compiler
    options="-m$mode -std=c++17 -O3 -fno-trapping-math -o$exefilename"
  elif [ $compiler -eq 3 ] ; then
    # Intel compiler for Linux, legacy
    options="-m$mode -std=c++17 -O3 -fno-trapping-math -fp-model precise -o$exefilename"    
  elif [ $compiler -eq 4 ] ; then
    # Intel compiler for Linux, clang based
    options="-m$mode -std=c++17 -O3 -fno-trapping-math -fp-model precise -o$exefilename"
  elif [ $compiler -eq 10 ] ; then
    # MS compiler for Windows VS2019
    os=1
    exefilename=xx.exe
    options="/O2 /std:c++17 /Fe$exefilename /arch:AVX2"
  elif [ $compiler -eq 11 ] ; then
    # Intel compiler for Windows
    os=1
    options="/O3 /std:c++17 /Fe$exefilename /fp:precise"
    exefilename=xx.exe
  fi
}


getInstructionSet() {
  # Find highest instruction set supported by current CPU
  # compile get_instruction_set.cpp
  if [ $compiler -eq 1 ] ; then
      # Gnu compiler
      eval g++ -msse2 -std=c++17 -I$include get_instruction_set.cpp -oiset$exefilename
      g++ --version
      g++ --version >> $outfile
      
  elif [ $compiler -eq 2 ] ; then
      # Clang compiler
      eval clang -msse2 -std=c++17 -I$include get_instruction_set.cpp -oiset$exefilename
      clang --version
      clang --version >> $outfile
    
  elif [ $compiler -eq 3 ] ; then
      # Intel compiler for Linux, legacy
      eval icc -std=c++17 -I$include get_instruction_set.cpp -oiset$exefilename
      icc --version
      icc --version >> $outfile

  elif [ $compiler -eq 4 ] ; then
      # Intel compiler for Linux, clang based
      eval icpx -std=c++17 -I$include get_instruction_set.cpp -oiset$exefilename
      icpx --version
      icpx --version >> $outfile

  elif [ $compiler -eq 10 ] ; then
      # MS compiler for Windows
      eval cl.exe -I$include /std:c++17 get_instruction_set.cpp /Fe:iset$exefilename
      cl.exe
      cl.exe >> $outfile
          
  elif [ $compiler -eq 11 ] ; then
      # Intel compiler for Windows
      eval icl.exe -I$include /std:c++17 get_instruction_set.cpp /Fe:iset$exefilename
      icl --version
      icl --version >> $outfile
      
  else
      echo "Error: Unknown compiler" 
      exitcode=1
      return 1
  fi

  if [ $? -ne 0 ] ; then 
    echo "Failed compiling get_instruction_set.cpp\n" 
    exit 2    # exit
  fi

  # run the compiled get_instruction_set.cpp to get the max instruction set
  eval maxiset=`./iset$exefilename`
  maxiset=`echo "$maxiset" | tr -d '\r\n'`  # remove trailing line feed

  if [ $? -ne 0 ] ; then 
    echo "Failed executing get_instruction_set.cpp\n" 
    exit 2    # exit
  fi

  echo -e "max instruction set: $maxiset\n"
}


startOutputFile() {
# Start writing output file
  echo -e "Test of VCL on $testbench with $outfile \n" > "$outfile"
  date +%Y-%m-%d:%H:%M:%S >> $outfile
  echo -e "\n\n" >> $outfile
}
  

# Save system path
syspath=$PATH

# Initialize variables to default values
mode=64
testbench=testbench1.cpp
include=../src2
outfile=/dev/null
seed=0
compiler=0
maxiset=
setCompiler 1


############################################
# Loop through the lines of the input file:
############################################

oldIFS=$IFS
IFS=$'\n'

# The filename comes at the end of this loop
# (If you put it here with a pipe, the variables in the loop will be lost)
while read -r line0
do

  # remove quotation marks and windows carriage return
  line=`echo "$line0" | tr -d '\r"'`

  # remove comments beginning with #
  line=${line%%#*}

  # skip blank lines
  if [[ -z "${line//[ ,]}" ]] ; then continue ; fi

    # detect special lines
  if [ "${line:0:1}" = "$" ] ; then 
    # line begins with "$". 
    # remove commas
	line1=${line//,/}

    # Split by "="    
    IFS="="
    spliteq=(${line1:1})
    varname="${spliteq[0]}"
    value="${spliteq[1]}"
    varname=${varname// }
    value=${value// }
    #echo -e "varname:$varname value:$value \n"

    if [[ $varname == "outfile" ]] ; then
      # set name of new output file
      if [[ $outfile != $value ]] ; then
        outfile=$value
        startOutputFile
      fi
    elif [[ $varname == "mode" ]] ; then
      # set mode
      mode=$value
      if [ $mode -ne 32 -a $mode -ne 64 ] ; then 
        echo "Error: Wrong mode: $mode" >> $outfile
        exit 1
      fi  
	   setCompiler "$compiler"
    elif [[ $varname == "testbench" ]] ; then
      # set testbench file
      testbench=$value   
    elif [[ $varname == "include" ]] ; then
      # set include directory
      include=$value 
    elif [[ $varname == "compiler" ]] ; then
      # set compiler
      setCompiler $value
    elif [[ $varname == "emulator" ]] ; then
      # set emulator
      emulator=$value
    elif [[ $varname == "compilermax" ]] ; then
      # set compilermax
      compilermax=$value      
    elif [[ $varname == "seed" ]] ; then
      # set compiler
      seed=$value
    else
      echo "Error: Unknown parameter $varname" >> $outfile
    fi    
  
    # reset field separator for read loop
    IFS=$'\n'
    continue
  fi

  # Check if outfile is started
  if [ -z "$outfile" ] ; then
    outfile="/dev/null"
    # startOutputFile
  fi 

  # Check if max instruction set has been detected
  if [ -z "$maxiset" ] ; then
    getInstructionSet
  fi 

  # General comma-separated line
  # Split by comma
  IFS=","
  splitline=($line)

  # reset field separator for read loop
  IFS=$'\n'

  
  # get fields:
  # 1. test case, integer or range, indicating which function or operator to test
  # 2. vector type for parameters
  # 3. vector type for return value
  # 4. instruction set (2=SSE2, 3=SSE3, 4=SSSE3, 5=SSE4.1, 6=SSE4.2,
  #    7=AVX, 8=AVX2+FMA, 9=AVX512F, 10=AVX512BW/DQ/VL
  # 5. function name
  # 6. indexes, separated by '+'
  testcases=${splitline[0]}
  vtypes=${splitline[1]}
  rtypes=${splitline[2]}
  instrsets=${splitline[3]}
  funcname=${splitline[4]}
  indexes=${splitline[5]}

  # default values
  if [ -z "$testcases" ] ; then
    testcases=1
  fi 
  if [ -z "$vtypes" ] ; then
    vtypes=Vec8i
  fi
  if [ -z "$rtypes" ] ; then
    rtypes=""  # Will be set to vtype
  fi
  if [ -z "$instrsets" ] ; then
    echo Error: Instruction set not specified, or a comma is missing
    exit 98
  fi

  # make array of test cases
  eval tcases=( $testcases )
  numtcases=${#tcases[@]}
  # make array of vector types
  eval vcases=( $vtypes )
  numvcases=${#vcases[@]}
  # make array of instruction sets
  eval icases=( $instrsets )
  numicases=${#icases[@]}
  
  # loops for parameter ranges
  for testcase in "${tcases[@]}" ; do
    if [ $numtcases -gt 1 ] ; then
      echo -e "* Testcase $testcase\n" >> $outfile
    fi
  
    for vtype in "${vcases[@]}" ; do
      if [ $numvcases -gt 1 ] ; then
        echo -e "** Vector type $vtype\n" >> $outfile
      fi  
  
      for instrset in "${icases[@]}" ; do

        if [ $numicases -gt 1 ] ; then
          echo -e "*** Instruction set $instrset\n" >> $outfile
        fi

        # parameters that can't have ranges
        rtype="$rtypes"
        
        # write to file
        echo -e "   test case $testcase $funcname, vector $vtype, instruction set $instrset, $testbench\n" >> $outfile
        
        # compilermax
        if [[ ! -z $compilermax ]] ; then
          if [[ $instrset -gt $compilermax ]] ; then
            echo -e "- skipped\n" >> $outfile
            continue
          fi
        fi          

        # compile testbench and run it
        compileAndRun
        
        # count successes
        counttests=$((counttests + 1))
    
      done
    done
  done 

  
IFS=$'\n'

done < $filename  # end line loop 


# reset field separator
IFS=$oldIFS

# statistics
endtime=`date +%s`
elapsedtime=$(($endtime - $starttime))
minutes=$(($elapsedtime/60))
seconds=$(($elapsedtime-($minutes*60)))


# output summary
echo -e "$counttests tests executed successfully in $minutes minutes $seconds seconds \n"
echo -e "$counttests tests executed successfully in $minutes minutes $seconds seconds \n" >> $outfile
