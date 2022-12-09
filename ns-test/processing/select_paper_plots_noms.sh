DST=paper-plots-noms
mkdir -p $DST
 
# cp plots/prio_test_bar/gw_snapshot_size:10/tot_mig_time.pdf paper-plots/exp-prio-tot-mig-time-10.pdf
# cp plots/prio_test_bar/gw_snapshot_size:50/tot_mig_time.pdf paper-plots/exp-prio-tot-mig-time-50.pdf
# cp plots/prio_test_bar/gw_snapshot_size:10/avg_bpq.pdf paper-plots/exp-prio-avg-bpq-10.pdf
# cp plots/prio_test_bar/gw_snapshot_size:50/avg_bpq.pdf paper-plots/exp-prio-avg-bpq-50.pdf

# cp plots/parallel_test/oversub:2/tot_mig_time.pdf paper-plots/exp-parallel-tot_mig_time-oversub-2.pdf
# cp plots/parallel_test/oversub:8/tot_mig_time.pdf paper-plots/exp-parallel-tot_mig_time-oversub-8.pdf
# cp plots/parallel_test/oversub:2/avg_bpq.pdf paper-plots/exp-parallel-avg-bpq-oversub-2.pdf
# cp plots/parallel_test/oversub:8/avg_bpq.pdf paper-plots/exp-parallel-avg-bpq-oversub-8.pdf

# cp plots/bg_test_2_bar_legend/bg_cdf:WebSearch/bg_100k_afct.pdf paper-plots/exp-mig-short-afct-websearch-load.pdf
# cp plots/bg_test_2_bar_legend/bg_cdf:WebSearch/bg_100k_ret.pdf paper-plots/exp-mig-short-ret-websearch-load.pdf
# cp plots/bg_test_2_bar_legend/bg_cdf:DataMining/bg_100k_afct.pdf paper-plots/exp-mig-short-afct-datamining-load.pdf
# cp plots/bg_test_2_bar_legend/bg_cdf:DataMining/bg_100k_ret.pdf paper-plots/exp-mig-short-ret-datamining-load.pdf

# cp plots/bg_test_2_bar/bg_cdf:WebSearch/bg_big_afct.pdf paper-plots/exp-mig-big-afct-websearch-load.pdf
# cp plots/bg_test_2_bar/bg_cdf:WebSearch/bg_big_ret.pdf paper-plots/exp-mig-big-ret-websearch-load.pdf
# cp plots/bg_test_2_bar/bg_cdf:DataMining/bg_big_afct.pdf paper-plots/exp-mig-big-afct-datamining-load.pdf
# cp plots/bg_test_2_bar/bg_cdf:DataMining/bg_big_ret.pdf paper-plots/exp-mig-big-ret-datamining-load.pdf

# cp plots/bg_test_3_bar/bg_cdf:WebSearch/vm_afct.pdf paper-plots/exp-mig-vm-afct-websearch-load.pdf
# cp plots/bg_test_3_bar/bg_cdf:DataMining/vm_afct.pdf paper-plots/exp-mig-vm-afct-datamining-load.pdf
# cp plots/bg_test_3_bar/bg_cdf:WebSearch/vm_99_ptk_in_flight_t.pdf paper-plots/exp-mig-vm-99-1-way-datamining-load.pdf
# cp plots/bg_test_3_bar/bg_cdf:DataMining/vm_99_ptk_in_flight_t.pdf paper-plots/exp-mig-vm-99-1-way-websearch-load.pdf

# cp plots/bg_test_4_bar/bg_cdf:WebSearch/bg_100k_afct.pdf paper-plots/exp-mig-short-afct-websearch-prio.pdf
# cp plots/bg_test_4_bar/bg_cdf:WebSearch/bg_100k_ret.pdf paper-plots/exp-mig-short-ret-websearch-prio.pdf
# cp plots/bg_test_4_bar/bg_cdf:DataMining/bg_100k_afct.pdf paper-plots/exp-mig-short-afct-datamining-prio.pdf
# cp plots/bg_test_4_bar/bg_cdf:DataMining/bg_100k_ret.pdf paper-plots/exp-mig-short-ret-datamining-prio.pdf

# cp plots/bg_test_5_bar/bg_cdf:WebSearch/vm_afct.pdf paper-plots/exp-mig-vm-afct-websearch-prio.pdf
# cp plots/bg_test_5_bar/bg_cdf:DataMining/vm_afct.pdf paper-plots/exp-mig-vm-afct-datamining-prio.pdf
# cp plots/bg_test_5_bar/bg_cdf:WebSearch/vm_99_ptk_in_flight_t.pdf paper-plots/exp-mig-vm-99-1-way-datamining-prio.pdf
# cp plots/bg_test_5_bar/bg_cdf:DataMining/vm_99_ptk_in_flight_t.pdf paper-plots/exp-mig-vm-99-1-way-websearch-prio.pdf
#-----------------------------------------------------------------

### 
cp exps/exp-cdf/NoMig-10/plots/flow_stats/average_in_flight_time_cdf_total.pdf $DST/exp-layering-inflight-cdf.pdf
cp exps/exp-cdf/NoMig-10/plots/flow_stats/average_buffered_time_cdf_total.pdf $DST/exp-layering-buffer-cdf.pdf
cp exps/prio-exp-cdf/1-level-10/plots/flow_stats/average_buffered_time_cdf_total.pdf $DST/exp-prio-buff-cdf.pdf
cp exps/prio-exp-cdf/1-level-10/plots/flow_stats/average_in_flight_time_cdf_total.pdf $DST/exp-prio-1-way-cdf.pdf

cp plots/prio_test/gw_snapshot_size:50/max_bpq.pdf $DST/exp-prio-max-bpq-50.pdf
cp plots/prio_test/gw_snapshot_size:50/tot_mig_time.pdf $DST/exp-prio-tot-mig-time-50.pdf

cp plots/parallel_test/gw_snapshot_size:10/tot_mig_time.pdf $DST/exp-parallel-tot-mig-time-10.pdf
cp plots/parallel_test/gw_snapshot_size:10/vm_99_ptk_in_flight_t.pdf $DST/exp-parallel-flight99-10.pdf
cp plots/parallel_test/gw_snapshot_size:50/tot_mig_time.pdf $DST/exp-parallel-tot-mig-time-50.pdf
cp plots/parallel_test/gw_snapshot_size:50/vm_99_ptk_in_flight_t.pdf $DST/exp-parallel-flight99-50.pdf

cp plots/bg_test_log/bg_cdf:WebSearch+prioritization:2-Levels/bg_100k_afct.pdf $DST/exp-bg-short-afct-websearch.pdf
cp plots/bg_test_no_legend_log/bg_cdf:WebSearch+prioritization:2-Levels/bg_big_afct.pdf $DST/exp-bg-long-afct-websearch.pdf
cp plots/bg_test_no_legend/bg_cdf:WebSearch+prioritization:2-Levels/vm_afct.pdf $DST/exp-bg-vnf-afct-websearch.pdf
cp plots/bg_test_no_legend/bg_cdf:WebSearch+prioritization:2-Levels/vm_99_ptk_in_flight_t.pdf $DST/exp-bg-vnf-flight99-websearch.pdf