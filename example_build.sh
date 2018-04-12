#!/bin/bash

rm -rf build
rm -rf install
mkdir build
mkdir install

cd build

found_host=false

#  -DCMAKE_CXX_FLAGS="-DUSE_TASK_TIMER -DCOUNT_FLOPS"

if [[ $NERSC_HOST = "edison" ]]; then
    found_host=true
    cmake ../ \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_INSTALL_PREFIX=../install \
          -DCMAKE_CXX_COMPILER=CC \
          -DCMAKE_C_COMPILER=cc \
          -DCMAKE_Fortran_COMPILER=ftn \
          -DCMAKE_EXE_LINKER_FLAGS="-dynamic" \
          -DMETIS_INCLUDES=$HOME/local/parmetis-4.0.3/metis/include \
          -DMETIS_LIBRARIES=$HOME/local/parmetis-4.0.3/build/Linux-x86_64/libmetis/libmetis.a \
          -DPARMETIS_INCLUDES=$HOME/local/parmetis-4.0.3/include \
          -DPARMETIS_LIBRARIES=$HOME/local/parmetis-4.0.3/build/Linux-x86_64/libparmetis/libparmetis.a \
          -DSCOTCH_INCLUDES=$HOME/local/scotch_6.0.4/include \
          -DSCOTCH_LIBRARIES="$HOME/local/scotch_6.0.4/lib/libscotch.a;$HOME/local/scotch_6.0.4/lib/libscotcherr.a;$HOME/local/scotch_6.0.4/lib/libptscotch.a;$HOME/local/scotch_6.0.4/lib/libptscotcherr.a"
fi

if [[ $NERSC_HOST = "cori" ]]; then
    found_host=true
    cmake ../ \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_INSTALL_PREFIX=../install \
          -DCMAKE_CXX_COMPILER=CC \
          -DCMAKE_C_COMPILER=cc \
          -DCMAKE_Fortran_COMPILER=ftn \
          -DCMAKE_EXE_LINKER_FLAGS="-dynamic" \
          -DMETIS_INCLUDES=$HOME/local/parmetis-4.0.3/metis/include \
          -DMETIS_LIBRARIES=$HOME/local/parmetis-4.0.3/build/Linux-x86_64/libmetis/libmetis.a

    # -DPARMETIS_INCLUDES=$HOME/local/parmetis-4.0.3/include \
        # -DPARMETIS_LIBRARIES=$HOME/local/parmetis-4.0.3/build/Linux-x86_64/libparmetis/libparmetis.a \
        # -DSCOTCH_INCLUDES=$HOME/local/scotch_6.0.4/include \
        # -DSCOTCH_LIBRARIES="$HOME/local/scotch_6.0.4/lib/libscotch.a;$HOME/local/scotch_6.0.4/lib/libscotcherr.a;$HOME/local/scotch_6.0.4/lib/libptscotch.a;$HOME/local/scotch_6.0.4/lib/libptscotcherr.a"
fi

if [[ $(hostname -s) = "xps13" ]]; then
    found_host=true
    cmake ../ \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_INSTALL_PREFIX=../install \
          -DMETIS_INCLUDES=/home/pieterg/local/parmetis-4.0.3/metis/include \
          -DMETIS_LIBRARIES=/home/pieterg/local/parmetis-4.0.3/build/Linux-x86_64/libmetis/libmetis.a

    # -DPARMETIS_INCLUDES=/home/pieterg/local/parmetis-4.0.3/include \
        # -DPARMETIS_LIBRARIES=/home/pieterg/local/parmetis-4.0.3/build/Linux-x86_64/libparmetis/libparmetis.a
fi

if [[ $(hostname -s) = "pieterg-X8DA3" ]]; then
    found_host=true
    COMBBLASHOME=/home/pieterg/LBL/STRUMPACK/CombBLAS_beta_16_1/CombBLAS_beta_16_1/build/
    cmake ../ \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_INSTALL_PREFIX=../install \
          -DSTRUMPACK_DEV_TESTING=ON \
          -DSTRUMPACK_C_INTERFACE=ON \
          -DSTRUMPACK_COUNT_FLOPS=ON \
          -DSTRUMPACK_TASK_TIMERS=ON \
          -DSTRUMPACK_USE_PARMETIS=ON \
          -DSTRUMPACK_USE_SCOTCH=ON \
          -DSTRUMPACK_USE_COMBBLAS=ON \
          -DCOMBBLAS_INCLUDES=/home/pieterg/LBL/STRUMPACK/CombBLAS_beta_16_1/CombBLAS_beta_16_1/ \
          -DCOMBBLAS_LIBRARIES="$COMBBLASHOME/libCommGridlib.a;$COMBBLASHOME/libHashlib.a;$COMBBLASHOME/libMemoryPoollib.a;$COMBBLASHOME/libmmiolib.a;$COMBBLASHOME/libMPIOplib.a;$COMBBLASHOME/libMPITypelib.a" \
          -DCMAKE_CXX_FLAGS="-Wall -Wfatal-errors -Wextra -Wno-unused-parameter" \
          -DMETIS_INCLUDES=$HOME/local/parmetis-4.0.3/metis/include \
          -DMETIS_LIBRARIES=$HOME/local/parmetis-4.0.3/build/Linux-x86_64/libmetis/libmetis.a \
          -DPARMETIS_INCLUDES=$HOME/local/parmetis-4.0.3/include \
          -DPARMETIS_LIBRARIES=$HOME/local/parmetis-4.0.3/build/Linux-x86_64/libparmetis/libparmetis.a \
          -DSCOTCH_INCLUDES=$HOME/local/scotch_6.0.4/include \
          -DSCOTCH_LIBRARIES="$HOME/local/scotch_6.0.4/lib/libscotch.a;$HOME/local/scotch_6.0.4/lib/libscotcherr.a;$HOME/local/scotch_6.0.4/lib/libptscotch.a;$HOME/local/scotch_6.0.4/lib/libptscotcherr.a"
fi


if ! $found_host; then
    echo "This machine was not recognized."
    echo "Open this file for examples on how to configure STRUMPACK."
    exit 1
fi

make install VERBOSE=1
cd examples
make -k
cd ../../
