export OMP_NUM_THREADS=1
export OMP_PLACES=threads
export OMP_PROC_BIND=spread



#n=10000
#wavelength=0.04
#aca=2
#ACC=1e-4
#LEAF=128
#nmpi=1
#export EXEC=./build/examples/EMCURV_HODLR
## mpirun -n 2 ${EXEC} ${n} ${aca} cob \
## --hss_leaf_size ${LEAF} --hss_rel_tol ${ACC} --hss_compression_algorithm original --hss_d0 128 --hss_max_rank 1024 2>&1 | tee emcurv_${n}_hodlr_strip.out
#srun -N 1 -n ${nmpi} -c 2 --cpu_bind=cores  ${EXEC} ${n} ${wavelength} ${aca} cob \
#--hss_leaf_size ${LEAF} --hss_rel_tol ${ACC} --hss_compression_algorithm original --hss_d0 128 --hss_max_rank 1024 2>&1 | tee emcurv_${n}_hodlr_butterfly_cavity_highK.out



#n=1000000
#aca=3
#ACC=1e-3
#LEAF=128
#nmpi=64
#export EXEC=./build/examples/ToeplitzQChem_HODLR
## mpirun -n 2 ${EXEC} ${n} ${aca} \
## --hss_leaf_size ${LEAF} --hss_rel_tol ${ACC} --hss_compression_algorithm original --hss_d0 128 --hss_max_rank 1024 2>&1 | tee toeplitzQchem.out
#srun -N 32 -n ${nmpi} -c 2 --cpu_bind=cores ${EXEC} ${n} ${aca} \
#--hss_leaf_size ${LEAF} --hss_rel_tol ${ACC} --hss_compression_algorithm original --hss_d0 128 --hss_max_rank 1024 2>&1 | tee toeplitzQchem_${n}_hss_N2.out


 h=0.1
 lambda=10
 ninc=1
 aca=2
 ACC=1e-2
 LEAF=128
 nmpi=2
 export EXEC=./build/examples/KernelRegressionMPI_HODLR
 export DATA=/project/projectdirs/m2957/liuyangz/my_research/ML/SUSY_Origin/susy_10Kn
 srun -n ${nmpi} -c 2 --cpu_bind=cores ${EXEC} ${DATA} 8 ${h} 1 cob ${lambda} ${nmpi} ${ninc} ${aca} test \
 --hss_leaf_size ${LEAF} --hss_rel_tol ${ACC} --hss_compression_algorithm original --hss_d0 128 --hss_max_rank 1024 2>&1 | tee -a susy_kernel10K.out
 # mpirun -n 2 ${EXEC} ${DATA} 8 ${h} 1 cob ${lambda} ${nmpi} ${ninc} ${aca} test \
 # --hss_leaf_size ${LEAF} --hss_rel_tol ${ACC} --hss_compression_algorithm original --hss_d0 128 --hss_max_rank 1024 2>&1 | tee -a susy_kernel10K.out
