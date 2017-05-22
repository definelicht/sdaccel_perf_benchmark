#!/usr/bin/env python3
import argparse
import csv
import itertools
import json
import numpy as np
import os
import re
import sys

def peak_utilization(board, costs, ops, maxLut=1.0, maxDsp=1.0):
  utilization = {
      "dspCount": 0,
      "lutCount": 0}
  nInstances = 0
  while True:
    newDsp = utilization["dspCount"]
    newLut = utilization["lutCount"]
    for (opType, opName, opCore, count) in ops:
      newDsp += count*costs[opType][opName][opCore]["dsp"]
      newLut += count*costs[opType][opName][opCore]["lut"]
    if (newDsp > board["compute"]["dspCount"]*maxDsp or
        newLut > board["compute"]["lutCount"]*maxLut):
      break
    utilization["dspCount"] = newDsp
    utilization["lutCount"] = newLut
    nInstances += 1
  return nInstances, utilization

def generate_permutations(costs, ops):
  permutations = []
  options = {}
  for typeName, typeVal in ops.items():
    for opName, opVal in typeVal.items():
      if type(opVal) == dict:
        key = (typeName, opName)
        options[key] = []
        for coreName, count in opVal.items():
          options[key].append((typeName, opName, coreName, count))
      else:
        key = (typeName, opName)
        options[key] = []
        for coreName in costs[typeName][opName]:
          options[key].append((typeName, opName, coreName, opVal))
  return list(itertools.product(*options.values()))

def utilization(boardPath, opsPath, costsPath, clock, maxlut, maxdsp,
                verbose=True):

  with open(boardPath, "r") as boardPath:
    board = json.load(boardPath)

  with open(opsPath, "r") as opsPath:
    ops = json.load(opsPath)

  with open(costsPath, "r") as costsPath:
    costs = json.load(costsPath)

  # MHz to Hz
  clock = 1e6 * clock

  if maxlut <= 0 or maxlut > 1:
    raise ValueError("maxlut must be in the interval ]0:1]")

  if maxdsp <= 0 or maxdsp > 1:
    raise ValueError("maxdsp must be in the interval ]0:1]")

  # Optimize between all permutations of different IP cores for the specified
  # operations
  permutations = generate_permutations(costs, ops)
  nInstances = 0
  bestPerm = None
  for perm in permutations:
    testInstances, testUtilization = peak_utilization(
        board, costs, perm, maxlut, maxdsp)
    if testInstances > nInstances:
      nInstances = testInstances
      utilization = testUtilization
      bestPerm = perm
  nOps = 0
  for (_, _, _, count) in bestPerm:
    nOps += nInstances*count
  peak = int(clock*nOps)

  if verbose:

    print("Operation counts:")
    for op, dtype, core, count in bestPerm:
      opStr = op + "/" + dtype + (("/" + core) if core else "")
      print("  {}: {} instances".format(opStr, count*nInstances))
    print("Total: {} ops/cycle".format(nOps))
    print("Peak: {:.1f} Gops/s at {} MHz".format(1e-9*peak, 1e-6*clock))
    print("Utilization:" +
          "\n  {} / {} DSP ({:.1f}%, {:.1f}% of available)".format(
              utilization["dspCount"], board["compute"]["dspCount"],
              1e2*utilization["dspCount"]/board["compute"]["dspCount"],
              1e2*utilization["dspCount"]/(maxdsp*board["compute"]["dspCount"])) +
          "\n  {} / {} LUT ({:.1f}%, {:.1f}% of available)".format(
              utilization["lutCount"], board["compute"]["lutCount"],
              1e2*utilization["lutCount"]/board["compute"]["lutCount"],
              1e2*utilization["lutCount"]/(maxlut*board["compute"]["lutCount"])))

if __name__ == "__main__":

  parser = argparse.ArgumentParser()
  parser.add_argument(
      "board",
      help="Path to JSON file containing board resource specifications.")
  parser.add_argument(
      "ops",
      help="Path to JSON file specifying which operations to maximize for.")
  parser.add_argument("-costs", default="costs/Virtex7.costs",
                      help="Path to JSON file specifying operation costs.")
  parser.add_argument("-clock", default=300, type=float,
                      help="Clock frequency in MHz.")
  parser.add_argument("-maxlut", default=1, type=float,
                      help="Maximum fraction of LUTs that can be used.")
  parser.add_argument("-maxdsp", default=1, type=float,
                      help="Maximum fraction of DSPs that can be used.")

  args = parser.parse_args()

  utilization(args.board, args.ops, args.costs, args.clock, args.maxlut,
              args.maxdsp)

