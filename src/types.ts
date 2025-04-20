/**
 * Improved documentation for VoicemeeterType
 * 'basic' - Voicemeeter Standard with 3 inputs/outputs
 * 'banana' - Voicemeeter Banana with 5 inputs/outputs
 * 'potato' - Voicemeeter Potato with 8 inputs/outputs
 */
export type VoicemeeterType = 'basic' | 'banana' | 'potato';

/**
 * Interface for Voicemeeter client configuration
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
 * Complete parameter interface with improved documentation
 */
export interface VoicemeeterParameters {
  /** Strip parameters for input channels */
  strip: {
    [index: number]: {
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
    }
  };
}