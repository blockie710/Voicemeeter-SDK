/**
 * Utility functions for the Voicemeeter SDK
 */

import { VoicemeeterType } from './types';
import { STRIP_COUNT, BUS_COUNT } from './constants';

/**
 * Validates if a value is within the specified range
 * @param value The value to validate
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @returns Value clamped to the specified range
 */
export function clamp(value: number, min: number, max: number): number {
  if (typeof value !== 'number' || isNaN(value)) {
    throw new Error('Value must be a valid number');
  }
  return Math.min(Math.max(value, min), max);
}

/**
 * Formats parameter names according to Voicemeeter's convention
 * @param name Base parameter name
 * @param index Channel index
 * @returns Formatted parameter name
 */
export function formatParameterName(name: string, index: number): string {
  if (typeof name !== 'string' || name.trim() === '') {
    throw new Error('Parameter name must be a non-empty string');
  }
  
  if (typeof index !== 'number' || isNaN(index) || index < 0) {
    throw new Error('Index must be a non-negative number');
  }
  
  return `${name}[${index}]`;
}

/**
 * Validates if a strip index is valid for the given Voicemeeter type
 * @param index Strip index to validate
 * @param type Voicemeeter type
 * @returns Whether the strip index is valid
 */
export function isValidStripIndex(index: number, type: VoicemeeterType): boolean {
  return Number.isInteger(index) && index >= 0 && index < STRIP_COUNT[type];
}

/**
 * Validates if a bus index is valid for the given Voicemeeter type
 * @param index Bus index to validate
 * @param type Voicemeeter type
 * @returns Whether the bus index is valid
 */
export function isValidBusIndex(index: number, type: VoicemeeterType): boolean {
  return Number.isInteger(index) && index >= 0 && index < BUS_COUNT[type];
}

/**
 * Converts a dB value to a linear gain value
 * @param dB Decibel value
 * @returns Linear gain value
 */
export function dbToLinear(dB: number): number {
  return Math.pow(10, dB / 20);
}

/**
 * Converts a linear gain value to a dB value
 * @param linear Linear gain value
 * @returns Decibel value
 */
export function linearToDb(linear: number): number {
  if (linear <= 0) return -Infinity;
  return 20 * Math.log10(linear);
}

/**
 * Converts a boolean value to 0 or 1
 * @param value Boolean value to convert
 * @returns 0 for false, 1 for true
 */
export function booleanToNumber(value: boolean): number {
  return value ? 1 : 0;
}

/**
 * Converts a 0 or 1 value to boolean
 * @param value Number to convert
 * @returns false for 0, true for any other number
 */
export function numberToBoolean(value: number): boolean {
  return value !== 0;
}

/**
 * Delays execution for the specified number of milliseconds
 * @param ms Milliseconds to delay
 * @returns Promise that resolves after the delay
 */
export function delay(ms: number): Promise<void> {
  return new Promise(resolve => setTimeout(resolve, ms));
}