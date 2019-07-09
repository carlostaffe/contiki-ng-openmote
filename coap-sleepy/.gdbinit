add-auto-load-safe-path /home/default/Workspace/contiki-ng/apps/hello-world/.gdbinit
target remote :3333
monitor start
watch curr_instance.dag.state
b coap-sleepy.c:165
