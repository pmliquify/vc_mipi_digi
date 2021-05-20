#bin/bash
#

cd /sys/kernel/debug/tracing

# Set the function to trace
#echo v4l_streamon > set_graph_function

# Set Filter
echo 'v4l*' 'mxc_*' 'video_*' 'vb2_*' 'mipi_*' 'vc_sd_*' 'vc_core_*' 'vc_mod_*' 'vc_sen_*'  > set_ftrace_filter
#echo > set_ftrace_filter

# Set Filter for functions not to trace
#echo 'atomic_*' '*print*' 'generic*' 'scheduler*' 'run*' 'tick*' 'dummy*' 'wake*' \
#        'irq*' '__*' '_*' 'lpuart32*' 'notifier*' > set_ftrace_notrace

# Set Tracefunction
#echo nop > current_tracer
echo function_graph > current_tracer
#echo function > current_tracer

# Clear Trace buffer
echo  > trace
# Aktivate tracing
echo 1 > tracing_on

# Execute the command to trace
#v4l2-ctl -c exposure=1000
#v4l2-ctl -c gain=1000
#v4l2-ctl --set-fmt-video=pixelformat=RGGB,width=3840,height=3040
#v4l2-ctl --stream-mmap --stream-count=1 --stream-to=file.raw
#/home/root/test/vctest
/home/root/test/vcmipidemo -a -s 20001 -g 101

# Deactivate tracing
echo 0 > tracing_on

less /sys/kernel/debug/tracing/trace