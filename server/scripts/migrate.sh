#!/usr/bin/env bash

if ! [ $1 == '--env' ]
then
  echo 'Missing environment path'
else
  export $(cat $2 | xargs) > /dev/null
  node_modules/knex/bin/cli.js migrate:latest
fi
