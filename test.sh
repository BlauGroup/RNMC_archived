cp ./test_materials/rn.sqlite ./test_materials/rn_copy.sqlite

./build/RNMC --database=./test_materials/rn_copy.sqlite --number_of_simulations=1000 --base_seed=1000 --thread_count=8 --step_cutoff=200

sql='SELECT seed, step, reaction_id FROM trajectories ORDER BY seed ASC, step ASC;'

sqlite3 ./test_materials/rn_with_trajectories.sqlite "${sql}" > ./test_materials/trajectories
sqlite3 ./test_materials/rn_copy.sqlite "${sql}" > ./test_materials/copy_trajectories

if  cmp ./test_materials/trajectories ./test_materials/copy_trajectories; then
    echo "trajectories same"
else
    echo "trajectories different"

rm ./test_materials/rn_copy.sqlite
rm ./test_materials/trajectories
rm ./test_materials/copy_trajectories
