#!/bin/sh
./gameboy BROKER CAUGHT_POKEMON 1 OK
./gameboy BROKER CAUGHT_POKEMON 2 FAIL

./gameboy BROKER NEW_POKEMON Pikachu 2 3 1

./gameboy BROKER CATCH_POKEMON Onyx 4 5

./gameboy SUSCRIPTOR NEW_POKEMON 10

./gameboy BROKER CATCH_POKEMON Charmander 4 5
