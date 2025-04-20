/**
 * Constants used by the Voicemeeter SDK
 */

/**
 * Maximum number of strips (input channels) for each Voicemeeter type
 */
export const STRIP_COUNT = {
  basic: 3, // 2 physical + 1 virtual
  banana: 5, // 3 physical + 2 virtual
  potato: 8, // 5 physical + 3 virtual
};

/**
 * Maximum number of buses (output channels) for each Voicemeeter type
 */
export const BUS_COUNT = {
  basic: 2, // 1 physical + 1 virtual
  banana: 5, // 3 physical + 2 virtual
  potato: 8, // 5 physical + 3 virtual
};

/**
 * Default polling interval for parameter updates in milliseconds
 */
export const DEFAULT_POLLING_INTERVAL = 100;

/**
 * Parameter path prefixes for different parameter types
 */
export const PARAMETER_PATHS = {
  STRIP: 'Strip',
  BUS: 'Bus',
  BUTTON: 'Button',
  VBAN: 'VBAN',
};

/**
 * Error messages for consistent error reporting
 */
export const ERROR_MESSAGES = {
  NOT_CONNECTED: 'Not connected to Voicemeeter. Call login() first.',
  INVALID_PARAMETER: 'Invalid parameter name.',
  INVALID_VALUE: 'Invalid parameter value.',
  CONNECTION_FAILED: 'Failed to connect to Voicemeeter. Make sure it is installed and running.',
  NOT_INSTALLED: 'Voicemeeter is not installed.',
  INITIALIZATION_FAILED: 'Failed to initialize Voicemeeter remote API.',
  PARAMETER_SET_FAILED: 'Failed to set parameter value.',
  PARAMETER_GET_FAILED: 'Failed to get parameter value.',
};