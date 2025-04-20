/**
 * Constants defining the maximum number of strips (inputs) for each Voicemeeter type
 */
export const VOICEMEETER_STRIP_COUNT = {
  basic: 3,
  banana: 5,
  potato: 8
};

/**
 * Constants defining the maximum number of buses (outputs) for each Voicemeeter type
 */
export const VOICEMEETER_BUS_COUNT = {
  basic: 2,
  banana: 5,
  potato: 8
};

/**
 * Error message constants for consistent error reporting
 */
export const ERROR_MESSAGES = {
  NOT_CONNECTED: 'Not connected to Voicemeeter. Call login() first.',
  INVALID_PARAM: 'Invalid parameter name.',
  INVALID_VALUE: 'Invalid parameter value.',
  CONNECTION_FAILED: 'Failed to connect to Voicemeeter. Make sure it is installed and running.',
  PARAMETER_SET_FAILED: 'Failed to set parameter.',
  PARAMETER_GET_FAILED: 'Failed to get parameter value.'
};