Red="\033[0;31m"          # Red
Green="\033[0;32m"        # Green
Color_Off="\033[0m"       # Text Reset

cp ./test_materials/initial_state.sqlite ./test_materials/initial_state_copy.sqlite

./RNMC --reaction_database=./test_materials/rn.sqlite --initial_state_database=./test_materials/initial_state_copy.sqlite --number_of_simulations=1000 --base_seed=1000 --thread_count=8 --step_cutoff=200 --gc_interval=2 --gc_threshold=1

sql='SELECT seed, step, reaction_id FROM trajectories ORDER BY seed ASC, step ASC;'

sqlite3 ./test_materials/initial_state_with_trajectories.sqlite "${sql}" > ./test_materials/trajectories
sqlite3 ./test_materials/initial_state_copy.sqlite "${sql}" > ./test_materials/copy_trajectories

if  cmp ./test_materials/trajectories ./test_materials/copy_trajectories > /dev/null; then
    echo -e "${Green} passed: no difference in trajectories ${Color_Off}"
    RC=0
else
    echo -e "${Red} failed: difference in trajectories ${Color_Off}"
    RC=1
fi

rm ./test_materials/initial_state_copy.sqlite
rm ./test_materials/trajectories
rm ./test_materials/copy_trajectories
exit $RC
