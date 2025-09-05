#!/usr/bin/env node
import fs from "fs";
import path from "path";
import { Headers } from "./ClangEnums.js";

// Get the command line arguments
const llvmFolder = process.argv[2];
const outputFolder = process.argv[3];

// Determine compiler from the provided LLVM_DIR (points to .../lib/cmake/llvm)
// `/usr/lib/llvm-${llvmVersion}/lib/cmake/llvm`;
const llvmLibCmake = path.resolve(llvmFolder);
const llvmPrefix = path.resolve(llvmLibCmake, "../../..");
const compilerCmd = path.join(llvmPrefix, "bin", process.platform === "win32" ? "clang++.exe" : "clang++");
const cppEnumsOutputFolder = path.join(outputFolder, "ClangEnums");
const javaEnumsOutputFolder = path.join(outputFolder, "java_enums");

console.log("Using LLVM folder '" + llvmFolder + "'");
console.log("Using compiler '" + compilerCmd + "'");
console.log("Generating enums to folder '" + outputFolder + "'");

fs.mkdirSync(cppEnumsOutputFolder, { recursive: true });
fs.mkdirSync(javaEnumsOutputFolder, { recursive: true });

for (const header of Headers) {
  header.process(compilerCmd, llvmFolder);
  header.generateCode(cppEnumsOutputFolder);
  header.generateJavaEnums(javaEnumsOutputFolder);
}
