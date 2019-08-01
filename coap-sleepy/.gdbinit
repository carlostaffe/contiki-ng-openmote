add-auto-load-safe-path /home/default/Workspace/contiki-ng/apps/hello-world/.gdbinit
target remote :3333
monitor start
watch curr_instance.dag.state
# b coap-sleepy.c:204 # inicio main thread
# keep_mac_on
b coap-sleepy.c:140 if nbr->state!=1
commands
  print nbr->state
  print nbr->ipaddr
  print nbr->isrouter
  print rv
  continue
end

b coap-sleepy.c:140 if nbr->state==1
commands
  print nbr->state
  print nbr->ipaddr
  print nbr->isrouter
  print rv
end
