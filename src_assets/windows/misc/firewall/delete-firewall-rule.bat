@echo off

set RULE_NAME=ApolloReady

rem Delete the rule
netsh advfirewall firewall delete rule name=%RULE_NAME%
