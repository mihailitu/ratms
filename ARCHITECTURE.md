# RATMS Production Architecture - Two Approaches

This document outlines two architectural approaches for building a production-ready Real-time Adaptive Traffic Management System.

---

## Approach 1: Modular Monolith (Recommended for MVP)

**Philosophy**: Single application with clear module boundaries, easier to develop and deploy initially, can evolve to microservices later.

### Directory Structure

```
ratms/
├── README.md
├── ARCHITECTURE.md
├── docker-compose.yml
├── .env.example
│
├── backend/                          # Main application server
│   ├── src/
│   │   ├── main.cpp                  # Application entry point
│   │   ├── api/                      # REST API endpoints
│   │   │   ├── routes/
│   │   │   │   ├── traffic_data.cpp  # Traffic data endpoints
│   │   │   │   ├── traffic_lights.cpp # Light control endpoints
│   │   │   │   ├── simulation.cpp    # Simulation endpoints
│   │   │   │   ├── optimization.cpp  # GA optimization endpoints
│   │   │   │   └── config.cpp        # Configuration endpoints
│   │   │   ├── middleware/
│   │   │   │   ├── auth.cpp          # Authentication
│   │   │   │   ├── rate_limit.cpp    # Rate limiting
│   │   │   │   └── logging.cpp       # Request logging
│   │   │   └── server.h/cpp          # HTTP server (crow/restinio)
│   │   │
│   │   ├── core/                     # Core simulation engine
│   │   │   ├── simulator.h/cpp       # (existing)
│   │   │   ├── road.h/cpp            # (existing)
│   │   │   ├── vehicle.h/cpp         # (existing)
│   │   │   ├── trafficlight.h/cpp    # (existing)
│   │   │   └── metrics.h/cpp         # (existing)
│   │   │
│   │   ├── optimization/             # GA optimization module
│   │   │   ├── genetic_algorithm.h/cpp  # (existing)
│   │   │   ├── traffic_controller.h/cpp # (existing)
│   │   │   └── optimizer_service.h/cpp  # Async optimization service
│   │   │
│   │   ├── data/                     # Data collection & processing
│   │   │   ├── collectors/
│   │   │   │   ├── base_collector.h  # Abstract collector interface
│   │   │   │   ├── web_stats_collector.cpp   # Web statistics scraper
│   │   │   │   ├── camera_collector.cpp      # Video feed processor (future)
│   │   │   │   ├── sensor_collector.cpp      # IoT sensor reader (future)
│   │   │   │   └── synthetic_collector.cpp   # Traffic demand generator (current)
│   │   │   ├── processors/
│   │   │   │   ├── data_cleaner.cpp          # Data validation
│   │   │   │   ├── aggregator.cpp            # Time-series aggregation
│   │   │   │   └── pattern_detector.cpp      # Traffic pattern analysis
│   │   │   └── storage/
│   │   │       ├── timeseries_db.cpp         # Time-series storage (InfluxDB)
│   │   │       └── metadata_db.cpp           # PostgreSQL/SQLite
│   │   │
│   │   ├── hardware/                 # Traffic light hardware interface
│   │   │   ├── controller/
│   │   │   │   ├── light_controller.h        # Abstract interface
│   │   │   │   ├── mqtt_controller.cpp       # MQTT protocol
│   │   │   │   ├── modbus_controller.cpp     # Modbus RTU/TCP
│   │   │   │   ├── snmp_controller.cpp       # SNMP protocol
│   │   │   │   └── simulator_controller.cpp  # Virtual (simulation only)
│   │   │   ├── state_manager.cpp             # Track light states
│   │   │   └── health_monitor.cpp            # Hardware health checks
│   │   │
│   │   ├── mapping/                  # Map & network management
│   │   │   ├── osm_importer.h/cpp    # (existing)
│   │   │   ├── network_builder.cpp   # Build road network
│   │   │   └── intersection_detector.cpp
│   │   │
│   │   ├── config/                   # Configuration management
│   │   │   ├── config_manager.cpp    # Load/save configs
│   │   │   ├── validation.cpp        # Config validation
│   │   │   └── schemas/              # JSON schemas
│   │   │       ├── network.schema.json
│   │   │       ├── demand.schema.json
│   │   │       └── optimization.schema.json
│   │   │
│   │   └── utils/                    # Utilities
│   │       ├── logger.h/cpp          # (existing)
│   │       ├── time_utils.cpp        # Time handling
│   │       └── metrics_exporter.cpp  # Prometheus metrics
│   │
│   ├── tests/                        # Unit & integration tests
│   │   ├── unit/
│   │   ├── integration/
│   │   └── fixtures/
│   │
│   └── CMakeLists.txt
│
├── frontend/                         # Web dashboard
│   ├── public/
│   │   └── index.html
│   ├── src/
│   │   ├── App.tsx                   # Main app component
│   │   ├── pages/
│   │   │   ├── Dashboard.tsx         # Real-time overview
│   │   │   ├── MapView.tsx           # Interactive map
│   │   │   ├── Analytics.tsx         # Historical analytics
│   │   │   ├── LightControl.tsx      # Manual light control
│   │   │   ├── Optimization.tsx      # GA configuration & results
│   │   │   └── Settings.tsx          # System configuration
│   │   ├── components/
│   │   │   ├── Map/
│   │   │   │   ├── TrafficMap.tsx    # Leaflet/Mapbox map
│   │   │   │   ├── RoadSegment.tsx   # Road visualization
│   │   │   │   ├── TrafficLight.tsx  # Light marker
│   │   │   │   └── VehicleMarker.tsx # Vehicle marker
│   │   │   ├── Charts/
│   │   │   │   ├── TimeSeries.tsx    # Traffic flow charts
│   │   │   │   ├── Heatmap.tsx       # Congestion heatmap
│   │   │   │   └── GAConvergence.tsx # GA fitness plot
│   │   │   ├── Controls/
│   │   │   │   ├── LightPanel.tsx    # Light control panel
│   │   │   │   ├── SimulationControl.tsx
│   │   │   │   └── OptimizationPanel.tsx
│   │   │   └── Common/
│   │   │       ├── Header.tsx
│   │   │       ├── Sidebar.tsx
│   │   │       └── StatusIndicator.tsx
│   │   ├── services/
│   │   │   ├── api.ts                # API client
│   │   │   ├── websocket.ts          # Real-time updates
│   │   │   └── auth.ts               # Authentication
│   │   ├── store/                    # State management (Redux/Zustand)
│   │   │   ├── trafficSlice.ts
│   │   │   ├── lightsSlice.ts
│   │   │   └── simulationSlice.ts
│   │   └── types/
│   │       ├── traffic.d.ts
│   │       └── api.d.ts
│   ├── package.json
│   └── vite.config.ts
│
├── database/                         # Database migrations & seeds
│   ├── migrations/
│   │   ├── 001_initial_schema.sql
│   │   ├── 002_add_traffic_data.sql
│   │   └── 003_add_optimization_results.sql
│   └── seeds/
│       └── sample_data.sql
│
├── config/                           # Application configuration
│   ├── default.yaml                  # Default config
│   ├── development.yaml              # Dev overrides
│   ├── production.yaml               # Prod overrides
│   ├── networks/                     # Network definitions
│   │   ├── test_network.json
│   │   └── munich_downtown.json
│   └── demand_profiles/              # Traffic demand profiles
│       ├── weekday_morning.json
│       └── weekend.json
│
├── scripts/                          # Utility scripts
│   ├── import_osm.py                 # Import OSM data
│   ├── generate_traffic.py           # Generate synthetic traffic
│   ├── benchmark.sh                  # Performance testing
│   └── deploy.sh                     # Deployment script
│
├── docs/                             # Documentation
│   ├── api/                          # API documentation
│   │   ├── openapi.yaml              # OpenAPI spec
│   │   └── endpoints.md
│   ├── setup/
│   │   ├── installation.md
│   │   └── configuration.md
│   └── architecture/
│       ├── data_flow.md
│       └── deployment.md
│
└── deployments/                      # Deployment configs
    ├── docker/
    │   ├── Dockerfile.backend
    │   ├── Dockerfile.frontend
    │   └── docker-compose.yml
    └── kubernetes/
        ├── deployment.yaml
        ├── service.yaml
        └── ingress.yaml
```

### Technology Stack

**Backend:**
- C++20 with CMake
- HTTP Server: Crow or RESTinio
- Database: PostgreSQL (metadata) + InfluxDB (time-series)
- Message Queue: Redis (for job queue)
- WebSocket: uWebSockets

**Frontend:**
- React 18 + TypeScript
- Vite (build tool)
- TailwindCSS (styling)
- Leaflet/Mapbox GL (maps)
- Recharts (charts)
- Socket.io-client (real-time)

**Infrastructure:**
- Docker + Docker Compose
- Nginx (reverse proxy)
- Redis (cache + queue)
- Prometheus + Grafana (monitoring)

### Communication Flow

```
[Web Browser]
     ↓ HTTP/WS
[Nginx Reverse Proxy]
     ↓
[Backend API Server] ←→ [PostgreSQL]
     ↓                ←→ [InfluxDB]
     ↓                ←→ [Redis Queue]
     ↓
[Core Modules]
  ├─ Simulation Engine
  ├─ GA Optimizer (async jobs)
  ├─ Data Collectors
  └─ Hardware Controllers
     ↓
[Traffic Light Hardware] (MQTT/Modbus)
```

### Advantages
✅ Simpler deployment (single binary + frontend)
✅ Easier development and debugging
✅ Lower operational complexity
✅ Shared code and dependencies
✅ Good performance (in-process communication)
✅ Can refactor to microservices later

### Disadvantages
❌ All components scale together
❌ Single point of failure
❌ Harder to use different languages per module

---

## Approach 2: Microservices Architecture (Recommended for Scale)

**Philosophy**: Distributed system with independent services, better for scaling and team separation.

### Directory Structure

```
ratms/
├── README.md
├── ARCHITECTURE.md
├── docker-compose.yml
├── kubernetes/                       # K8s manifests for all services
│
├── services/                         # Microservices
│   │
│   ├── api-gateway/                  # API Gateway (Node.js/Go)
│   │   ├── src/
│   │   │   ├── routes/
│   │   │   ├── middleware/
│   │   │   └── proxy.ts
│   │   ├── Dockerfile
│   │   └── package.json
│   │
│   ├── traffic-data-service/         # Data collection service
│   │   ├── src/
│   │   │   ├── main.py               # Python (good for scraping/ML)
│   │   │   ├── collectors/
│   │   │   │   ├── web_scraper.py    # Scrape traffic statistics
│   │   │   │   ├── api_fetcher.py    # Fetch from traffic APIs
│   │   │   │   └── synthetic_gen.py  # Generate synthetic data
│   │   │   ├── processors/
│   │   │   │   ├── cleaner.py
│   │   │   │   ├── aggregator.py
│   │   │   │   └── pattern_analysis.py
│   │   │   ├── storage/
│   │   │   │   └── influx_client.py
│   │   │   └── api/
│   │   │       └── rest_api.py       # FastAPI
│   │   ├── requirements.txt
│   │   └── Dockerfile
│   │
│   ├── simulation-service/           # Core simulation (C++)
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── api/
│   │   │   │   └── grpc_server.cpp   # gRPC API
│   │   │   ├── core/                 # Existing simulator code
│   │   │   │   ├── simulator.h/cpp
│   │   │   │   ├── road.h/cpp
│   │   │   │   ├── vehicle.h/cpp
│   │   │   │   └── metrics.h/cpp
│   │   │   └── messaging/
│   │   │       └── kafka_producer.cpp
│   │   ├── proto/
│   │   │   └── simulation.proto      # gRPC definitions
│   │   ├── CMakeLists.txt
│   │   └── Dockerfile
│   │
│   ├── optimization-service/         # GA optimizer (C++)
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── api/
│   │   │   │   └── grpc_server.cpp
│   │   │   ├── optimizer/
│   │   │   │   ├── genetic_algorithm.h/cpp (existing)
│   │   │   │   ├── traffic_controller.h/cpp (existing)
│   │   │   │   └── optimizer_worker.cpp
│   │   │   └── messaging/
│   │   │       ├── kafka_consumer.cpp  # Job queue
│   │   │       └── kafka_producer.cpp  # Results
│   │   ├── proto/
│   │   │   └── optimization.proto
│   │   ├── CMakeLists.txt
│   │   └── Dockerfile
│   │
│   ├── hardware-controller-service/  # Traffic light control (Python/Go)
│   │   ├── src/
│   │   │   ├── main.go               # Go (good for network protocols)
│   │   │   ├── controllers/
│   │   │   │   ├── mqtt_controller.go
│   │   │   │   ├── modbus_controller.go
│   │   │   │   └── snmp_controller.go
│   │   │   ├── state/
│   │   │   │   └── state_manager.go
│   │   │   └── api/
│   │   │       └── grpc_server.go
│   │   ├── go.mod
│   │   └── Dockerfile
│   │
│   ├── config-service/               # Configuration management (Node.js)
│   │   ├── src/
│   │   │   ├── api/
│   │   │   │   └── rest_api.ts       # Express/Fastify
│   │   │   ├── storage/
│   │   │   │   └── postgres_client.ts
│   │   │   └── validation/
│   │   │       └── schemas.ts
│   │   ├── package.json
│   │   └── Dockerfile
│   │
│   └── web-dashboard/                # Frontend (React)
│       ├── (same structure as Approach 1)
│       └── Dockerfile
│
├── shared/                           # Shared code/configs
│   ├── proto/                        # Shared protobuf definitions
│   │   ├── common.proto
│   │   ├── traffic.proto
│   │   └── events.proto
│   └── schemas/                      # JSON schemas
│       └── *.json
│
├── infrastructure/                   # Infrastructure as code
│   ├── terraform/                    # Cloud infrastructure
│   │   ├── main.tf
│   │   ├── networking.tf
│   │   └── kubernetes.tf
│   ├── helm/                         # Helm charts
│   │   └── ratms/
│   │       ├── Chart.yaml
│   │       ├── values.yaml
│   │       └── templates/
│   └── monitoring/
│       ├── prometheus/
│       │   └── prometheus.yml
│       └── grafana/
│           └── dashboards/
│
└── scripts/                          # DevOps scripts
    ├── deploy_all.sh
    ├── scale_service.sh
    └── health_check.sh
```

### Technology Stack per Service

| Service | Language | Framework | Database | Protocol |
|---------|----------|-----------|----------|----------|
| API Gateway | Node.js/Go | Express/Gin | Redis (cache) | HTTP/WS |
| Traffic Data | Python | FastAPI | InfluxDB | REST + Kafka |
| Simulation | C++ | gRPC | - | gRPC + Kafka |
| Optimization | C++ | gRPC | Redis (jobs) | gRPC + Kafka |
| Hardware Control | Go | gRPC | - | gRPC + MQTT |
| Config | Node.js | Fastify | PostgreSQL | REST |
| Dashboard | React+TS | Vite | - | HTTP/WS |

### Communication Flow

```
[Web Browser]
     ↓ HTTP/WS
[API Gateway (Node.js)]
     ↓
     ├─→ [Traffic Data Service (Python)] ←→ [InfluxDB]
     │        ↓ publish
     │   [Kafka: traffic.data topic]
     │
     ├─→ [Simulation Service (C++)]
     │        ↓ consume: traffic.data
     │        ↓ publish: simulation.results
     │   [Kafka]
     │
     ├─→ [Optimization Service (C++)]
     │        ↓ consume: optimization.jobs
     │        ↓ publish: optimization.results
     │   [Kafka]
     │
     ├─→ [Hardware Controller (Go)]
     │        ↓ MQTT/Modbus
     │   [Physical Traffic Lights]
     │
     └─→ [Config Service (Node.js)] ←→ [PostgreSQL]
```

### Message Queue Topics (Kafka)

- `traffic.data.raw` - Raw traffic measurements
- `traffic.data.processed` - Cleaned/aggregated data
- `simulation.requests` - Simulation job requests
- `simulation.results` - Simulation outputs
- `optimization.jobs` - GA optimization jobs
- `optimization.results` - Optimized parameters
- `lights.commands` - Traffic light commands
- `lights.status` - Traffic light status updates
- `system.events` - System-wide events

### Advantages
✅ Independent scaling per service
✅ Technology flexibility (Python for ML, Go for networking, C++ for performance)
✅ Team autonomy (different teams own different services)
✅ Fault isolation (one service crash doesn't kill everything)
✅ Independent deployment
✅ Better for distributed deployment (multi-region)

### Disadvantages
❌ Complex deployment and orchestration
❌ Network latency between services
❌ Distributed debugging is harder
❌ More operational overhead (monitoring, logging)
❌ Eventual consistency challenges

---

## Comparison Matrix

| Aspect | Modular Monolith | Microservices |
|--------|------------------|---------------|
| **Initial Development** | ⭐⭐⭐⭐⭐ Faster | ⭐⭐⭐ Slower |
| **Deployment Complexity** | ⭐⭐⭐⭐⭐ Simple | ⭐⭐ Complex |
| **Scalability** | ⭐⭐⭐ Limited | ⭐⭐⭐⭐⭐ Excellent |
| **Technology Flexibility** | ⭐⭐ Limited | ⭐⭐⭐⭐⭐ High |
| **Debugging** | ⭐⭐⭐⭐⭐ Easy | ⭐⭐ Difficult |
| **Operational Cost** | ⭐⭐⭐⭐⭐ Low | ⭐⭐ High |
| **Team Size** | 1-5 developers | 5+ developers |
| **Best For** | MVP, Small-Medium scale | Large scale, Multiple teams |

---

## Recommended Migration Path

### Phase 1: Start with Modular Monolith (Months 1-6)
- Build all features in single application
- Use clear module boundaries
- Deploy as one service
- Validate product-market fit

### Phase 2: Extract High-Load Services (Months 6-12)
- Extract GA optimizer to separate service (CPU intensive)
- Extract data collector to separate service (I/O intensive)
- Keep simulation engine in monolith

### Phase 3: Full Microservices (Year 2+)
- Split remaining services
- Implement service mesh (Istio/Linkerd)
- Add distributed tracing
- Scale independently

---

## Implementation Priority

### Immediate (MVP):
1. ✅ Core simulation engine (DONE)
2. ✅ GA optimizer (DONE)
3. Synthetic traffic data generator (enhance existing)
4. Basic web dashboard (real-time monitoring)
5. Virtual traffic light controller (simulation)

### Phase 2 (Production Ready):
6. Web statistics collector (scrape real traffic data)
7. Configuration management UI
8. Historical analytics dashboard
9. REST API for all operations
10. Database persistence

### Phase 3 (Hardware Integration):
11. MQTT traffic light controller
12. Camera/sensor data collector
13. Real-time hardware monitoring
14. Fault tolerance and failover

### Phase 4 (Advanced Features):
15. Machine learning demand prediction
16. Multi-city support
17. Mobile app
18. Public API for third parties
