/**
 * Utilities for testing the Voicemeeter SDK
 */

import { VoicemeeterType } from './types';

/**
 * Creates a mock Voicemeeter instance for testing without actual Voicemeeter installation
 * @param type Type of Voicemeeter to mock
 * @returns Mock object with in-memory parameter storage
 */
export function createMockVoicemeeter(type: VoicemeeterType = 'basic') {
  const parameters = new Map<string, number | string>();
  
  return {
    type,
    isConnected: true,
    
    setParameterFloat: (name: string, value: number): void => {
      parameters.set(name, value);
    },
    
    getParameterFloat: (name: string): number => {
      const value = parameters.get(name);
      if (typeof value === 'number') {
        return value;
      }
      return 0;
    },
    
    setParameterString: (name: string, value: string): void => {
      parameters.set(name, value);
    },
    
    getParameterString: (name: string): string => {
      const value = parameters.get(name);
      if (typeof value === 'string') {
        return value;
      }
      return '';
    },
    
    getAllParameters: () => {
      return Object.fromEntries(parameters.entries());
    },
    
    clearParameters: () => {
      parameters.clear();
    }
  };
}

/**
 * Helper function to delay execution for testing async operations
 * @param ms Milliseconds to wait
 * @returns Promise that resolves after the specified time
 */
export function delay(ms: number): Promise<void> {
  return new Promise(resolve => setTimeout(resolve, ms));
}
