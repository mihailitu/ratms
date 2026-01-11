import { server } from './mocks/server';

export default function globalTeardown() {
  // Close MSW server after all tests
  server.close();
  console.log('MSW server closed');
}
