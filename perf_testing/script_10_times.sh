cd ../
make
cd perf_testing

# Define argument configurations (each element is a full config string)
configs=(
    "--LF 2 --N 10000"
    "--LF 3 --N 10000"
    "--LFC 2 --N 10000"
    "--LFC 3 --N 10000"
    "--MLF 2 1 3 --N 10000"
    "--MDBSCAN --N 10000"
)

learn="perf_testing/myspace-20000.txt"
base="english-6.txt"
target="Xato-net-100k.txt"

learn_size=$(wc -l < "../$learn")
base_size=$(wc -l < $base)
target_size=$(wc -l < $target)

# Prepare summary output file
summary_output="summary_results.txt"
echo "$learn($learn_size) -> $base($base_size) -> $target($target_size)" >> "$summary_output"
echo >> "$summary_output"

skipped_loops=0

# Loop through configurations
for config in "${configs[@]}"; do
    total_hits=0
    total_time_ns=0
    total_rules=0

    echo "========================================="
    echo "Running configuration: $config"
    echo "Learn size: $learn_size"
    echo "Base size: $base_size"
    echo "Target size: $target_size"
    echo "========================================="

    for i in {1..3}; do
        echo "Running iteration $i for config: $config" 
        # Start timing
        start_time=$(date +%s%N)

        # Run rule generator with current config (suppress output)
        cd ../
        ./fastruleforge --i $learn --o rules.rule $config > /dev/null 2>&1
        cd perf_testing

        # End timing
        end_time=$(date +%s%N)
        duration_ns=$((end_time - start_time))
        total_time_ns=$((total_time_ns + duration_ns))

        # Copy rules
        cp ../rules.rule rules.rule

        # Count rules
        rule_count=$(wc -l < rules.rule)
        total_rules=$((total_rules + rule_count))

        # Run hashcat and collect unique hits
        hashcat -r rules.rule $base --stdout | grep -Fxf $target | sort -u > hits.txt
        hits=$(wc -l < hits.txt)
        total_hits=$((total_hits + hits))

        # if config contains --RANDOM or --no-randomize in the text, end the loop 
        if [[ $config == *"--RANDOM"* || $config == *"--no-randomize"* ]]; then
            skipped_loops=1
            break
        fi
        
    done

    if [ $skipped_loops -eq 1 ]; then
        average_hits=$(echo "scale=2; $total_hits" | bc)
        average_time_ms=$(echo "scale=2; $total_time_ns / 1000000000" | bc)
        average_rules=$(echo "scale=2; $total_rules" | bc)
    else
        average_hits=$(echo "scale=2; $total_hits / 3" | bc)
        average_time_ms=$(echo "scale=2; $total_time_ns / 3000000000" | bc)
        average_rules=$(echo "scale=2; $total_rules / 3" | bc)
    fi  

    # Compute averages
    

    echo "********** RESULTS FOR CONFIG **********"
    echo "$config"
    echo "Average hits over 5 runs: $average_hits"
    echo "Average time per fastruleforge run: $average_time_ms s"
    echo "Average ruleset size (lines in rules.rule): $average_rules"
    echo "***************************************"

    # Append to summary
    printf "%-20s | Avg Hits: %-6s | Avg Time: %-8s ms | Avg Rules: %-6s\n" "$config" "$average_hits" "$average_time_ms" "$average_rules" >> "$summary_output"
done
printf "=============================================================================\n" >> "$summary_output"

# Print the summary
echo "=========== SUMMARY ==========="
echo "$learn($learn_size) -> $base($base_size) -> $target($target_size)"
echo "==============================="
cat "$summary_output"
echo "==============================="
