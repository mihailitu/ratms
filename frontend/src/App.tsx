import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import Layout from './components/Layout';
import Dashboard from './pages/Dashboard';
import Simulations from './pages/Simulations';
import SimulationDetail from './pages/SimulationDetail';
import MapView from './pages/MapView';
import Optimization from './pages/Optimization';

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
        </Routes>
      </Layout>
    </Router>
  );
}

export default App;
