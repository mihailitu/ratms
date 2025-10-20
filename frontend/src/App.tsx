import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import Layout from './components/Layout';
import Dashboard from './pages/Dashboard';
import Simulations from './pages/Simulations';
import SimulationDetail from './pages/SimulationDetail';
import MapView from './pages/MapView';
import Optimization from './pages/Optimization';
import Analytics from './pages/Analytics';

function App() {
  return (
    <Router>
      <Layout>
        <Routes>
          <Route path="/" element={<Dashboard />} />
          <Route path="/simulations" element={<Simulations />} />
          <Route path="/simulations/:id" element={<SimulationDetail />} />
          <Route path="/map" element={<MapView />} />
          <Route path="/optimization" element={<Optimization />} />
          <Route path="/analytics" element={<Analytics />} />
        </Routes>
      </Layout>
    </Router>
  );
}

export default App;
