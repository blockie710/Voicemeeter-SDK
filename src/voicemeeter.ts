/**
 * Main implementation of the Voicemeeter SDK
 */

import { 
  VoicemeeterType, 
  VoicemeeterClientConfig, 
  VoicemeeterParameters,
  ParameterChangeEvent,
  VoicemeeterErrorCode
} from './types';
import { 
  STRIP_COUNT, 
  BUS_COUNT, 
  DEFAULT_POLLING_INTERVAL,
  ERROR_MESSAGES,
  PARAMETER_PATHS
} from './constants';
import { 
  formatParameterName, 
  isValidStripIndex, 
  isValidBusIndex,
  delay,
  clamp
} from './utils';

/**
 * Main class for interacting with Voicemeeter
 */
export class VoicemeeterClient {
  private type: VoicemeeterType;
  private pollingInterval: number;
  private autoConnect: boolean;
  private isConnected: boolean = false;
  private polling: boolean = false;
  private pollTimer: NodeJS.Timeout | null = null;
  private eventListeners: Map<string, ((event: ParameterChangeEvent) => void)[]> = new Map();
  
  /**
   * Creates a new VoicemeeterClient instance
   * @param config Configuration options
   */
  constructor(config?: VoicemeeterClientConfig) {
    this.type = config?.type || 'basic';
    this.autoConnect = config?.autoConnect || false;
    this.pollingInterval = config?.pollingInterval || DEFAULT_POLLING_INTERVAL;
    
    if (this.autoConnect) {
      this.login().catch(err => {
        console.error('Failed to auto-connect to Voicemeeter:', err);
      });
    }
  }

  /**
   * Connects to the Voicemeeter application
   * @returns Promise that resolves when connected
   * @throws Error if connection fails
   */
  async login(): Promise<void> {
    if (this.isConnected) {
      return;
    }
    
    try {
      // Implementation would connect to Voicemeeter here
      // using platform-specific bindings
      
      // Wait for Voicemeeter to initialize
      await this.waitForVoicemeeter();
      
      this.isConnected = true;
      
      if (this.pollingInterval > 0) {
        this.startPolling();
      }
    } catch (error) {
      throw new Error(`${ERROR_MESSAGES.CONNECTION_FAILED} ${error instanceof Error ? error.message : String(error)}`);
    }
  }

  /**
   * Disconnects from the Voicemeeter application
   * @returns Promise that resolves when disconnected
   */
  async logout(): Promise<void> {
    if (!this.isConnected) {
      return;
    }
    
    try {
      this.stopPolling();
      
      // Implementation would disconnect from Voicemeeter here
      
      this.isConnected = false;
    } catch (error) {
      throw new Error(`Failed to logout from Voicemeeter: ${error instanceof Error ? error.message : String(error)}`);
    }
  }

  /**
   * Checks if Voicemeeter is running
   * @returns True if Voicemeeter is running, false otherwise
   */
  isVoicemeeterRunning(): boolean {
    // Implementation would check if Voicemeeter is running
    return this.isConnected;
  }

  /**
   * Gets the type of Voicemeeter installation
   * @returns The Voicemeeter type
   */
  getVoicemeeterType(): VoicemeeterType {
    return this.type;
  }

  /**
   * Sets a float parameter value
   * @param paramName Parameter name
   * @param value Parameter value
   * @throws Error if not connected or parameter is invalid
   */
  setParameterFloat(paramName: string, value: number): void {
    this.ensureConnected();
    
    if (typeof paramName !== 'string' || paramName.trim() === '') {
      throw new Error(ERROR_MESSAGES.INVALID_PARAMETER);
    }
    
    if (typeof value !== 'number' || isNaN(value)) {
      throw new Error(ERROR_MESSAGES.INVALID_VALUE);
    }
    
    // Implementation would set parameter in Voicemeeter here
  }

  /**
   * Gets a float parameter value
   * @param paramName Parameter name
   * @returns Parameter value
   * @throws Error if not connected or parameter is invalid
   */
  getParameterFloat(paramName: string): number {
    this.ensureConnected();
    
    if (typeof paramName !== 'string' || paramName.trim() === '') {
      throw new Error(ERROR_MESSAGES.INVALID_PARAMETER);
    }
    
    // Implementation would get parameter from Voicemeeter here
    return 0;
  }

  /**
   * Sets a string parameter value
   * @param paramName Parameter name
   * @param value Parameter value
   * @throws Error if not connected or parameter is invalid
   */
  setParameterString(paramName: string, value: string): void {
    this.ensureConnected();
    
    if (typeof paramName !== 'string' || paramName.trim() === '') {
      throw new Error(ERROR_MESSAGES.INVALID_PARAMETER);
    }
    
    if (typeof value !== 'string') {
      throw new Error(ERROR_MESSAGES.INVALID_VALUE);
    }
    
    // Implementation would set parameter in Voicemeeter here
  }

  /**
   * Gets a string parameter value
   * @param paramName Parameter name
   * @returns Parameter value
   * @throws Error if not connected or parameter is invalid
   */
  getParameterString(paramName: string): string {
    this.ensureConnected();
    
    if (typeof paramName !== 'string' || paramName.trim() === '') {
      throw new Error(ERROR_MESSAGES.INVALID_PARAMETER);
    }
    
    // Implementation would get parameter from Voicemeeter here
    return '';
  }

  /**
   * Sets a strip parameter value
   * @param stripIndex Strip index
   * @param paramName Parameter name
   * @param value Parameter value
   * @throws Error if not connected or parameters are invalid
   */
  setStripParameter(stripIndex: number, paramName: string, value: number | string): void {
    if (!isValidStripIndex(stripIndex, this.type)) {
      throw new Error(`Invalid strip index: ${stripIndex}`);
    }
    
    const fullParamName = formatParameterName(`${PARAMETER_PATHS.STRIP}.${paramName}`, stripIndex);
    
    if (typeof value === 'number') {
      this.setParameterFloat(fullParamName, value);
    } else {
      this.setParameterString(fullParamName, value);
    }
  }

  /**
   * Gets a strip parameter value
   * @param stripIndex Strip index
   * @param paramName Parameter name
   * @returns Parameter value
   * @throws Error if not connected or parameters are invalid
   */
  getStripParameter(stripIndex: number, paramName: string): number | string {
    if (!isValidStripIndex(stripIndex, this.type)) {
      throw new Error(`Invalid strip index: ${stripIndex}`);
    }
    
    const fullParamName = formatParameterName(`${PARAMETER_PATHS.STRIP}.${paramName}`, stripIndex);
    
    try {
      return this.getParameterFloat(fullParamName);
    } catch (error) {
      // If getting as float fails, try as string
      return this.getParameterString(fullParamName);
    }
  }

  /**
   * Sets a bus parameter value
   * @param busIndex Bus index
   * @param paramName Parameter name
   * @param value Parameter value
   * @throws Error if not connected or parameters are invalid
   */
  setBusParameter(busIndex: number, paramName: string, value: number | string): void {
    if (!isValidBusIndex(busIndex, this.type)) {
      throw new Error(`Invalid bus index: ${busIndex}`);
    }
    
    const fullParamName = formatParameterName(`${PARAMETER_PATHS.BUS}.${paramName}`, busIndex);
    
    if (typeof value === 'number') {
      this.setParameterFloat(fullParamName, value);
    } else {
      this.setParameterString(fullParamName, value);
    }
  }

  /**
   * Gets a bus parameter value
   * @param busIndex Bus index
   * @param paramName Parameter name
   * @returns Parameter value
   * @throws Error if not connected or parameters are invalid
   */
  getBusParameter(busIndex: number, paramName: string): number | string {
    if (!isValidBusIndex(busIndex, this.type)) {
      throw new Error(`Invalid bus index: ${busIndex}`);
    }
    
    const fullParamName = formatParameterName(`${PARAMETER_PATHS.BUS}.${paramName}`, busIndex);
    
    try {
      return this.getParameterFloat(fullParamName);
    } catch (error) {
      // If getting as float fails, try as string
      return this.getParameterString(fullParamName);
    }
  }

  /**
   * Gets all current parameters
   * @returns All Voicemeeter parameters
   * @throws Error if not connected
   */
  getAllParameters(): VoicemeeterParameters {
    this.ensureConnected();
    
    const params: VoicemeeterParameters = { strip: {}, bus: {} };
    
    // Get all strip parameters
    const stripCount = STRIP_COUNT[this.type];
    for (let i = 0; i < stripCount; i++) {
      params.strip[i] = {
        mute: this.getParameterFloat(formatParameterName(`${PARAMETER_PATHS.STRIP}.Mute`, i)),
        gain: this.getParameterFloat(formatParameterName(`${PARAMETER_PATHS.STRIP}.Gain`, i)),
        mono: this.getParameterFloat(formatParameterName(`${PARAMETER_PATHS.STRIP}.Mono`, i)),
        solo: this.getParameterFloat(formatParameterName(`${PARAMETER_PATHS.STRIP}.Solo`, i)),
        label: this.getParameterString(formatParameterName(`${PARAMETER_PATHS.STRIP}.Label`, i))
      };
    }
    
    // Get all bus parameters
    const busCount = BUS_COUNT[this.type];
    for (let i = 0; i < busCount; i++) {
      params.bus[i] = {
        mute: this.getParameterFloat(formatParameterName(`${PARAMETER_PATHS.BUS}.Mute`, i)),
        gain: this.getParameterFloat(formatParameterName(`${PARAMETER_PATHS.BUS}.Gain`, i)),
        mono: this.getParameterFloat(formatParameterName(`${PARAMETER_PATHS.BUS}.Mono`, i)),
        label: this.getParameterString(formatParameterName(`${PARAMETER_PATHS.BUS}.Label`, i))
      };
    }
    
    return params;
  }

  /**
   * Add an event listener for parameter changes
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
   * Remove an event listener
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
   * Starts polling for parameter changes
   */
  private startPolling(): void {
    if (this.polling) {
      return;
    }
    
    this.polling = true;
    
    const pollFunction = async () => {
      if (!this.polling) {
        return;
      }
      
      try {
        // Implementation would check for parameter changes here
        // and emit events for any changes
      } catch (error) {
        console.error('Error during polling:', error);
      }
      
      this.pollTimer = setTimeout(pollFunction, this.pollingInterval);
    };
    
    pollFunction();
  }

  /**
   * Stops polling for parameter changes
   */
  private stopPolling(): void {
    if (!this.polling) {
      return;
    }
    
    this.polling = false;
    
    if (this.pollTimer !== null) {
      clearTimeout(this.pollTimer);
      this.pollTimer = null;
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
   * Ensures that the client is connected to Voicemeeter
   * @throws Error if not connected
   */
  private ensureConnected(): void {
    if (!this.isConnected) {
      throw new Error(ERROR_MESSAGES.NOT_CONNECTED);
    }
  }

  /**
   * Waits for Voicemeeter to be fully initialized
   * @param maxAttempts Maximum number of attempts
   * @param delayMs Delay between attempts in milliseconds
   */
  private async waitForVoicemeeter(maxAttempts = 30, delayMs = 100): Promise<void> {
    for (let i = 0; i < maxAttempts; i++) {
      // Implementation would check if Voicemeeter is fully initialized
      
      // For demo purposes, we'll simulate success after a few attempts
      if (i > 3) {
        return;
      }
      
      await delay(delayMs);
    }
    
    throw new Error('Timed out waiting for Voicemeeter to initialize');
  }
}