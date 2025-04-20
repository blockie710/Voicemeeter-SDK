/**
 * Validates if a value is within the specified range
 * @param value The value to validate
 * @param min Minimum allowed value
 * @param max Maximum allowed value
 * @returns Value clamped to the specified range
 */
export function validateRange(value: number, min: number, max: number): number {
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