import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import Layout from './components/Layout';
import ProductionDashboard from './pages/ProductionDashboard';
import MapView from './pages/MapView';
import Analytics from './pages/Analytics';

function App() {
  return (
    <Router>
      <Layout>
        <Routes>
          <Route path="/" element={<ProductionDashboard />} />
          <Route path="/map" element={<MapView />} />
          <Route path="/analytics" element={<Analytics />} />
          {/* Redirect old routes to home */}
          <Route path="/production" element={<Navigate to="/" replace />} />
          <Route path="/simulations" element={<Navigate to="/" replace />} />
          <Route path="/simulations/:id" element={<Navigate to="/" replace />} />
          <Route path="/optimization" element={<Navigate to="/" replace />} />
          <Route path="/statistics" element={<Navigate to="/" replace />} />
        </Routes>
      </Layout>
    </Router>
  );
}

export default App;
