/**
 * Testing utilities for the Voicemeeter SDK
 */

import { 
  VoicemeeterType, 
  VoicemeeterParameters,
  ParameterChangeEvent 
} from './types';
import { delay } from './utils';

/**
 * Mock implementation of the Voicemeeter client for testing
 */
export class MockVoicemeeterClient {
  private type: VoicemeeterType;
  private isConnected: boolean = false;
  private parameters: Map<string, number | string> = new Map();
  private eventListeners: Map<string, ((event: ParameterChangeEvent) => void)[]> = new Map();
  
  /**
   * Creates a new MockVoicemeeterClient
   * @param type Voicemeeter type to mock
   */
  constructor(type: VoicemeeterType = 'basic') {
    this.type = type;
  }
  
  /**
   * Simulates connecting to Voicemeeter
   */
  async login(): Promise<void> {
    // Simulate connection delay
    await delay(100);
    this.isConnected = true;
  }
  
  /**
   * Simulates disconnecting from Voicemeeter
   */
  async logout(): Promise<void> {
    await delay(50);
    this.isConnected = false;
  }
  
  /**
   * Checks if mock Voicemeeter is running
   */
  isVoicemeeterRunning(): boolean {
    return this.isConnected;
  }
  
  /**
   * Gets the type of mock Voicemeeter
   */
  getVoicemeeterType(): VoicemeeterType {
    return this.type;
  }
  
  /**
   * Sets a float parameter value in the mock
   * @param paramName Parameter name
   * @param value Parameter value
   */
  setParameterFloat(paramName: string, value: number): void {
    if (!this.isConnected) {
      throw new Error('Not connected to Voicemeeter');
    }
    
    this.parameters.set(paramName, value);
    this.emitEvent('parameterChanged', {
      parameterName: paramName,
      value: value
    });
  }
  
  /**
   * Gets a float parameter value from the mock
   * @param paramName Parameter name
   */
  getParameterFloat(paramName: string): number {
    if (!this.isConnected) {
      throw new Error('Not connected to Voicemeeter');
    }
    
    const value = this.parameters.get(paramName);
    
    if (typeof value !== 'number') {
      return 0;
    }
    
    return value;
  }
  
  /**
   * Sets a string parameter value in the mock
   * @param paramName Parameter name
   * @param value Parameter value
   */
  setParameterString(paramName: string, value: string): void {
    if (!this.isConnected) {
      throw new Error('Not connected to Voicemeeter');
    }
    
    this.parameters.set(paramName, value);
    this.emitEvent('parameterChanged', {
      parameterName: paramName,
      value: value
    });
  }
  
  /**
   * Gets a string parameter value from the mock
   * @param paramName Parameter name
   */
  getParameterString(paramName: string): string {
    if (!this.isConnected) {
      throw new Error('Not connected to Voicemeeter');
    }
    
    const value = this.parameters.get(paramName);
    
    if (typeof value !== 'string') {
      return '';
    }
    
    return value;
  }
  
  /**
   * Adds a parameter change event listener
   * @param eventName Event name
   * @param callback Callback function
   */
  addEventListener(eventName: string, callback: (event: ParameterChangeEvent) => void): void {
    if (!this.eventListeners.has(eventName)) {
      this.eventListeners.set(eventName, []);
    }
    
    this.eventListeners.get(eventName)!.push(callback);
  }
  
  /**
   * Removes a parameter change event listener
   * @param eventName Event name
   * @param callback Callback function to remove
   */
  removeEventListener(eventName: string, callback: (event: ParameterChangeEvent) => void): void {
    if (!this.eventListeners.has(eventName)) {
      return;
    }
    
    const listeners = this.eventListeners.get(eventName)!;
    const index = listeners.indexOf(callback);
    
    if (index !== -1) {
      listeners.splice(index, 1);
    }
  }
  
  /**
   * Emits an event to all registered listeners
   * @param eventName Event name
   * @param event Event data
   */
  private emitEvent(eventName: string, event: ParameterChangeEvent): void {
    if (!this.eventListeners.has(eventName)) {
      return;
    }
    
    for (const listener of this.eventListeners.get(eventName)!) {
      try {
        listener(event);
      } catch (error) {
        console.error('Error in event listener:', error);
      }
    }
  }
  
  /**
   * Clears all mock parameters
   */
  clearParameters(): void {
    this.parameters.clear();
  }
  
  /**
   * Sets multiple parameters at once
   * @param params Parameters to set
   */
  setParameters(params: Record<string, number | string>): void {
    for (const [key, value] of Object.entries(params)) {
      if (typeof value === 'number') {
        this.setParameterFloat(key, value);
      } else {
        this.setParameterString(key, value);
      }
    }
  }
}

/**
 * Creates a mock Voicemeeter client
 * @param type Type of Voicemeeter to mock
 */
export function createMockVoicemeeterClient(type: VoicemeeterType = 'basic'): MockVoicemeeterClient {
  return new MockVoicemeeterClient(type);
}
