import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import Layout from './components/Layout';
import Dashboard from './pages/Dashboard';
import Simulations from './pages/Simulations';
import SimulationDetail from './pages/SimulationDetail';
import MapView from './pages/MapView';
import Optimization from './pages/Optimization';
import Analytics from './pages/Analytics';
import Statistics from './pages/Statistics';
import ProductionDashboard from './pages/ProductionDashboard';

function App() {
  return (
    <Router>
      <Layout>
        <Routes>
          <Route path="/" element={<Dashboard />} />
          <Route path="/production" element={<ProductionDashboard />} />
          <Route path="/simulations" element={<Simulations />} />
          <Route path="/simulations/:id" element={<SimulationDetail />} />
          <Route path="/map" element={<MapView />} />
          <Route path="/optimization" element={<Optimization />} />
          <Route path="/analytics" element={<Analytics />} />
          <Route path="/statistics" element={<Statistics />} />
        </Routes>
      </Layout>
    </Router>
  );
}

export default App;
