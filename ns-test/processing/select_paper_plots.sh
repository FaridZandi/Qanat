mkdir -p paper-plots
 

cp exp-cdf/NoMig-10/plots/flow_stats/average_in_flight_time_cdf_total.pdf paper-plots/exp-layering-inflight-cdf.pdf
cp exp-cdf/NoMig-10/plots/flow_stats/average_buffered_time_cdf_total.pdf paper-plots/exp-layering-buffer-cdf.pdf
cp prio-exp-cdf/1-level-10/plots/flow_stats/average_buffered_time_cdf_total.pdf paper-plots/exp-prio-buff-cdf.pdf
cp prio-exp-cdf/1-level-10/plots/flow_stats/average_in_flight_time_cdf_total.pdf paper-plots/exp-prio-1-way-cdf.pdf

cp plots/prio_test_bar/gw_snapshot_size:10+cc_protocol:DCTCP/tot_mig_time.pdf paper-plots/exp-prio-tot-mig-time-10.pdf
cp plots/prio_test_bar/gw_snapshot_size:50+cc_protocol:DCTCP/tot_mig_time.pdf paper-plots/exp-prio-tot-mig-time-50.pdf
cp plots/prio_test_bar/gw_snapshot_size:10+cc_protocol:DCTCP/avg_bpq.pdf paper-plots/exp-prio-avg-bpq-10.pdf
cp plots/prio_test_bar/gw_snapshot_size:50+cc_protocol:DCTCP/avg_bpq.pdf paper-plots/exp-prio-avg-bpq-50.pdf


cp plots/parallel_test/oversub:2/tot_mig_time.pdf paper-plots/exp-parallel-tot_mig_time-oversub-2.pdf
cp plots/parallel_test/oversub:8/tot_mig_time.pdf paper-plots/exp-parallel-tot_mig_time-oversub-8.pdf
cp plots/parallel_test/oversub:2/avg_bpq.pdf paper-plots/exp-parallel-avg-bpq-oversub-2.pdf
cp plots/parallel_test/oversub:8/avg_bpq.pdf paper-plots/exp-parallel-avg-bpq-oversub-8.pdf


cp plots/bg_test_2_bar_legend/bg_cdf:WebSearch/bg_100k_afct.pdf paper-plots/exp-mig-short-afct-websearch-load.pdf
cp plots/bg_test_2_bar_legend/bg_cdf:WebSearch/bg_100k_ret.pdf paper-plots/exp-mig-short-ret-websearch-load.pdf
cp plots/bg_test_2_bar_legend/bg_cdf:DataMining/bg_100k_afct.pdf paper-plots/exp-mig-short-afct-datamining-load.pdf
cp plots/bg_test_2_bar_legend/bg_cdf:DataMining/bg_100k_ret.pdf paper-plots/exp-mig-short-ret-datamining-load.pdf

cp plots/bg_test_2_bar/bg_cdf:WebSearch/bg_big_afct.pdf paper-plots/exp-mig-big-afct-websearch-load.pdf
cp plots/bg_test_2_bar/bg_cdf:WebSearch/bg_big_ret.pdf paper-plots/exp-mig-big-ret-websearch-load.pdf
cp plots/bg_test_2_bar/bg_cdf:DataMining/bg_big_afct.pdf paper-plots/exp-mig-big-afct-datamining-load.pdf
cp plots/bg_test_2_bar/bg_cdf:DataMining/bg_big_ret.pdf paper-plots/exp-mig-big-ret-datamining-load.pdf

cp plots/bg_test_3_bar/bg_cdf:WebSearch/vm_afct.pdf paper-plots/exp-mig-vm-afct-websearch-load.pdf
cp plots/bg_test_3_bar/bg_cdf:DataMining/vm_afct.pdf paper-plots/exp-mig-vm-afct-datamining-load.pdf
cp plots/bg_test_3_bar/bg_cdf:WebSearch/vm_99_ptk_in_flight_t.pdf paper-plots/exp-mig-vm-99-1-way-datamining-load.pdf
cp plots/bg_test_3_bar/bg_cdf:DataMining/vm_99_ptk_in_flight_t.pdf paper-plots/exp-mig-vm-99-1-way-websearch-load.pdf

cp plots/bg_test_4_bar/bg_cdf:WebSearch/bg_100k_afct.pdf paper-plots/exp-mig-short-afct-websearch-prio.pdf
cp plots/bg_test_4_bar/bg_cdf:WebSearch/bg_100k_ret.pdf paper-plots/exp-mig-short-ret-websearch-prio.pdf
cp plots/bg_test_4_bar/bg_cdf:DataMining/bg_100k_afct.pdf paper-plots/exp-mig-short-afct-datamining-prio.pdf
cp plots/bg_test_4_bar/bg_cdf:DataMining/bg_100k_ret.pdf paper-plots/exp-mig-short-ret-datamining-prio.pdf

cp plots/bg_test_5_bar/bg_cdf:WebSearch/vm_afct.pdf paper-plots/exp-mig-vm-afct-websearch-prio.pdf
cp plots/bg_test_5_bar/bg_cdf:DataMining/vm_afct.pdf paper-plots/exp-mig-vm-afct-datamining-prio.pdf
cp plots/bg_test_5_bar/bg_cdf:WebSearch/vm_99_ptk_in_flight_t.pdf paper-plots/exp-mig-vm-99-1-way-datamining-prio.pdf
cp plots/bg_test_5_bar/bg_cdf:DataMining/vm_99_ptk_in_flight_t.pdf paper-plots/exp-mig-vm-99-1-way-websearch-prio.pdf


cp plots/latency_test_heatmap/gw_snapshot_size:10+migration_status:Migration/tot_mig_time.pdf paper-plots/exp-latency-heatmap-mig-time-10.pdf
cp plots/latency_test_heatmap/gw_snapshot_size:50+migration_status:Migration/tot_mig_time.pdf paper-plots/exp-latency-heatmap-mig-time-50.pdf

