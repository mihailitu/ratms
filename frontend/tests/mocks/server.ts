import { setupServer } from 'msw/node';
import { handlers } from './handlers';

// Setup MSW server for Node.js (used in Playwright tests)
export const server = setupServer(...handlers);
