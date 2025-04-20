/**
 * Type definitions for the Voicemeeter SDK
 */

/**
 * Defines the type of Voicemeeter installation
 */
export type VoicemeeterType = 'basic' | 'banana' | 'potato';

/**
 * Configuration options for the Voicemeeter client
 */
export interface VoicemeeterClientConfig {
  /** Type of Voicemeeter installation */
  type?: VoicemeeterType;
  /** Automatically attempt to connect on initialization */
  autoConnect?: boolean;
  /** Polling interval in milliseconds for parameter updates */
  pollingInterval?: number;
}

/**
 * Interface for strip (input channel) parameters
 */
export interface StripParameters {
  /** Mute state (0: not muted, 1: muted) */
  mute?: number;
  /** Mono state (0: not mono, 1: mono) */
  mono?: number;
  /** Solo state (0: not solo, 1: solo) */
  solo?: number;
  /** Channel label/name */
  label?: string;
  /** Gain value in dB (-60 to +12) */
  gain?: number;
  /** Pan value (-0.5 left, 0 center, 0.5 right) */
  pan?: number;
  /** EQ state (0: off, 1: on) */
  eq?: number;
  /** Reverb level (0.0 to 1.0) */
  reverb?: number;
  /** Delay level (0.0 to 1.0) */
  delay?: number;
  /** Fx1 level (0.0 to 1.0) */
  fx1?: number;
  /** Fx2 level (0.0 to 1.0) */
  fx2?: number;
}

/**
 * Interface for bus (output channel) parameters
 */
export interface BusParameters {
  /** Mute state (0: not muted, 1: muted) */
  mute?: number;
  /** Mono state (0: not mono, 1: mono) */
  mono?: number;
  /** Channel label/name */
  label?: string;
  /** Gain value in dB (-60 to +12) */
  gain?: number;
  /** EQ state (0: off, 1: on) */
  eq?: number;
  /** Mode (0-4 depending on Voicemeeter type) */
  mode?: number;
}

/**
 * Complete Voicemeeter parameters interface
 */
export interface VoicemeeterParameters {
  /** Strip (input channel) parameters indexed by strip number */
  strip: {
    [index: number]: StripParameters;
  };
  /** Bus (output channel) parameters indexed by bus number */
  bus: {
    [index: number]: BusParameters;
  };
}

/**
 * Interface for defining parameter change events
 */
export interface ParameterChangeEvent {
  /** Parameter name that changed */
  parameterName: string;
  /** New value of the parameter */
  value: number | string;
}

/**
 * Error codes returned by the Voicemeeter Remote API
 */
export enum VoicemeeterErrorCode {
  OK = 0,
  ERROR = -1,
  NOT_INITIALIZED = -2,
  ALREADY_INITIALIZED = -3,
  NOT_RUNNING = -4,
  ALREADY_RUNNING = -5,
}