import { server } from './mocks/server';

export default function globalSetup() {
  // Start MSW server before all tests
  server.listen({ onUnhandledRequest: 'warn' });
  console.log('MSW server started');
}
