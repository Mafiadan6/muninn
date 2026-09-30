# Security Policy

## Our Commitment to Security

At muninn, we take the security of our users and their devices very seriously. We are committed to developing and maintaining a robust and secure flashing tool for Samsung devices. Our design principles prioritize safety and validation to minimize risks associated with firmware flashing.

## Supported Versions

We currently provide security updates for the latest stable release of muninn. Users are strongly encouraged to always use the most up-to-date version of the tool to ensure they benefit from the latest security enhancements and bug fixes.

## Reporting a Vulnerability

We appreciate the efforts of security researchers and the community in helping us maintain a secure project. If you discover a security vulnerability in muninn, we kindly request that you report it to us privately.

**Please DO NOT open a public GitHub issue for security vulnerabilities.**

You can report the vulnerability through one of the following channels:

- **Preferred method**: Use the **GitHub Security Advisories** feature.  
  1. Navigate to the [muninn GitHub repository](https://github.com/Llucs/muninn).  
  2. Click on the **"Security"** tab.  
  3. Click on **"Report a vulnerability"**.  
  4. Fill out the form with as much detail as possible (description, reproduction steps, impact, and suggested fixes).

- **Alternative contact**: You can also email us directly at **c307lucas@gmail.com**.

We will acknowledge your report within 48 hours and provide a more detailed response within 7 days, indicating the next steps in handling your report.

## Our Security Philosophy

muninn is designed with the following security principles:

- **Strict Validation**: All firmware packages and device interactions undergo rigorous validation to prevent malicious or corrupted data from being flashed.
- **Minimal Privileges**: The tool operates with the minimum necessary privileges to perform its functions.
- **Transparency**: While we handle vulnerabilities privately, our development process is open, allowing for community scrutiny and contribution to overall security.
- **No Destructive Operations**: muninn explicitly avoids implementing inherently destructive or high-risk operations such as NAND erase or forced repartitioning, which could lead to irreversible device damage.

Thank you for helping us keep muninn secure and reliable.