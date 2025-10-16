import { Link, useLocation } from 'react-router-dom';

interface LayoutProps {
  children: React.ReactNode;
}

export default function Layout({ children }: LayoutProps) {
  const location = useLocation();

  const isActive = (path: string) => {
    return location.pathname === path;
  };

  const navLinkClass = (path: string) =>
    `px-3 py-2 rounded-md text-sm font-medium transition-colors ${
      isActive(path)
        ? 'bg-blue-700 text-white'
        : 'text-blue-100 hover:bg-blue-600 hover:text-white'
    }`;

  return (
    <div className="min-h-screen bg-gray-100">
      <nav className="bg-blue-600 shadow-lg">
        <div className="max-w-7xl mx-auto px-4">
          <div className="flex items-center justify-between h-16">
            <div className="flex items-center">
              <div className="flex-shrink-0">
                <h1 className="text-white text-xl font-bold">RATMS</h1>
              </div>
              <div className="ml-10 flex items-baseline space-x-4">
                <Link to="/" className={navLinkClass('/')}>
                  Dashboard
                </Link>
                <Link to="/simulations" className={navLinkClass('/simulations')}>
                  Simulations
                </Link>
                <Link to="/optimization" className={navLinkClass('/optimization')}>
                  Optimization
                </Link>
                <Link to="/map" className={navLinkClass('/map')}>
                  Map View
                </Link>
              </div>
            </div>
          </div>
        </div>
      </nav>
      <main>{children}</main>
    </div>
  );
}
