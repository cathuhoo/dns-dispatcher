dns-dispatcher
==============
Dispatcher is a DNS forwarder, which forwards DNS queries to different DNS resolvers 
according to local policy. The example of such policy file is following:

###### Example of a policy file ###############
# A policy file is a text file contains rules 
# A rule is a line include : <Source_ip_prefix, Destination_domain , Action>
# An action is <Operation [dns_resolver] >
# Operation = Drop | refuse | Forward 
#
# The following line is a rule to deny access to some bad guys
# For any IP in blacklist_ip.txt, and any target domain, Drop the DNS request and repy nothing  
blacklist_ip.txt| * | Drop 

# The following rule is to forward DNS request<from any IP, to any domain in blacklist_domain.txt> 
# to google's open DNS Resolver ( the information of "google" is defined in resolvers.txt
*| blacklist_domain.txt| Forward:google

# The following rule is to forward DNS request <from Tsinghua_ip, to domains in video.txt> 
# to a dns resolver in China Telecom  
tsinghua_ip.txt| video.txt| Forward:telecom

# The following rule is to forward DNS request <from CERNET_ip, to domains in video.txt> 
# to a dns resolver of  CCERT  
cernet_ip.txt| video.txt| Forward: ccert 

# For any other request, forward to a default DNS resolver, ccert
*| * | Forward: ccert 

