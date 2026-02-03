<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { Chart, registerables } from 'chart.js'

Chart.register(...registerables)

const chartCanvas = ref(null)
const memoryData = ref({ free_heap: 0, min_free_heap: 0 })
const HOUR_IN_MS = 60 * 60 * 1000 // 1 hour in milliseconds
const FETCH_INTERVAL = 10000 // 10 seconds
let chart = null
let intervalId = null

// Store data with timestamps
const dataPoints = [] // Array of { timestamp: Date, value: number }

const fetchMemory = async () => {
  try {
    const response = await fetch('/api/v1/system/memory')
    if (!response.ok) throw new Error('Failed to fetch memory data')
    
    const data = await response.json()
    console.log('[MemoryChart] Fetched data:', data)
    memoryData.value = data
    
    const now = new Date()
    const freeHeapKB = Math.round(data.free_heap / 1024)
    
    // Add new data point
    dataPoints.push({ timestamp: now, value: freeHeapKB })
    
    // Remove data points older than 1 hour
    const cutoffTime = new Date(now.getTime() - HOUR_IN_MS)
    while (dataPoints.length > 0 && dataPoints[0].timestamp < cutoffTime) {
      dataPoints.shift()
    }
    
    console.log('[MemoryChart] Data points:', dataPoints.length, 'Latest:', freeHeapKB)
    
    if (chart) {
      console.log('[MemoryChart] Updating chart...')
      updateChartData()
      console.log('[MemoryChart] Chart updated')
    } else {
      console.warn('[MemoryChart] Chart not initialized yet')
    }
  } catch (err) {
    console.error('Error fetching memory:', err)
  }
}

// Update chart with current data and smart labels
const updateChartData = () => {
  if (!chart) return
  
  // Prepare data for chart
  const labels = []
  const values = []
  
  for (let i = 0; i < dataPoints.length; i++) {
    const point = dataPoints[i]
    values.push(point.value)
    
    // Only show labels for first, last, and optionally some in between
    if (i === 0 || i === dataPoints.length - 1) {
      // First and last: show time
      labels.push(point.timestamp.toLocaleTimeString())
    } else if (dataPoints.length > 20 && i % Math.floor(dataPoints.length / 4) === 0) {
      // For longer datasets, show a few intermediate points
      labels.push(point.timestamp.toLocaleTimeString())
    } else {
      // Empty label for other points
      labels.push('')
    }
  }
  
  chart.data.labels = labels
  chart.data.datasets[0].data = values
  chart.update('default')
}

onMounted(async () => {
  // Create chart FIRST with empty data
  const ctx = chartCanvas.value.getContext('2d')
  chart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: [],
      datasets: [{
        label: 'Free Heap (kB)',
        data: [],
        borderColor: '#42b883',
        backgroundColor: 'rgba(66, 184, 131, 0.1)',
        tension: 0.4,
        fill: true
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: true,
          ticks: {
            color: '#aaa'
          },
          grid: {
            color: '#333'
          }
        },
        x: {
          ticks: {
            color: '#aaa',
            maxRotation: 45,
            minRotation: 45,
            autoSkip: false // Don't auto-skip since we control labels
          },
          grid: {
            color: '#333'
          }
        }
      },
      plugins: {
        legend: {
          labels: {
            color: '#fff'
          }
        },
        tooltip: {
          callbacks: {
            title: (context) => {
              // Show full timestamp in tooltip
              const index = context[0].dataIndex
              if (dataPoints[index]) {
                return dataPoints[index].timestamp.toLocaleString()
              }
              return ''
            }
          }
        }
      },
      animation: {
        duration: 0
      }
    }
  })
  
  // Now fetch initial data
  await fetchMemory()
  
  // Start interval
  intervalId = setInterval(fetchMemory, FETCH_INTERVAL)
})

onUnmounted(() => {
  if (intervalId) clearInterval(intervalId)
  if (chart) chart.destroy()
})

const leakMemory = async () => {
  try {
    const response = await fetch('/api/v1/system/leak', { method: 'POST' })
    if (!response.ok) throw new Error('Failed to leak memory')
    const data = await response.json()
    console.log('[MemoryChart] Leaked:', data)
    // Immediately fetch new memory data to show the effect
    await fetchMemory()
  } catch (err) {
    console.error('Error leaking memory:', err)
  }
}

const restartDevice = async () => {
  if (!confirm('Are you sure you want to restart the ESP32?')) return
  try {
    await fetch('/api/v1/system/restart', { method: 'POST' })
  } catch (err) {
    console.log('Device restarting...')
  }
}

const crashDevice = async () => {
  if (!confirm('Are you sure you want to crash the ESP32?')) return
  try {
    await fetch('/api/v1/system/crash', { method: 'POST' })
  } catch (err) {
    console.log('Device crashed...')
  }
}
</script>

<template>
  <div class="memory-section">
    <h2>Memory Status</h2>
    <div class="button-container">
      <button @click="leakMemory" class="leak-button">Leak Memory</button>
      <button @click="restartDevice" class="restart-button">Restart</button>
      <button @click="crashDevice" class="crash-button">Crash Me</button>
    </div>
    <div class="memory-stats">
      <div class="stat-item">
        <span class="stat-label">Current Free Heap:</span>
        <span class="stat-value">{{ Math.round(memoryData.free_heap / 1024).toLocaleString() }} kB</span>
      </div>
      <div class="stat-item">
        <span class="stat-label">Minimum Free Heap:</span>
        <span class="stat-value">{{ Math.round(memoryData.min_free_heap / 1024).toLocaleString() }} kB</span>
      </div>
    </div>
    <div class="chart-container">
      <canvas ref="chartCanvas"></canvas>
    </div>
  </div>
</template>

<style scoped>
.memory-section {
  margin-top: 2rem;
  width: 100%;
  max-width: 100%;
}

h2 {
  color: #42b883;
  margin-bottom: 1rem;
  text-align: center;
}

.memory-stats {
  background: #1a1a1a;
  border-radius: 8px;
  padding: 1rem;
  margin-bottom: 1rem;
  display: flex;
  justify-content: space-around;
  gap: 1rem;
  width: 100%;
}

.stat-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.5rem;
}

.stat-label {
  color: #aaa;
  font-size: 0.9rem;
}

.stat-value {
  color: #42b883;
  font-family: monospace;
  font-size: 1.1rem;
  font-weight: bold;
}

.button-container {
  text-align: center;
  margin-bottom: 1rem;
  display: flex;
  gap: 1rem;
  justify-content: center;
}

.leak-button,
.restart-button,
.crash-button {
  border: none;
  padding: 0.75rem 1.5rem;
  border-radius: 6px;
  font-size: 1rem;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.2s;
  color: white;
}

.leak-button {
  background: #ff4444;
}

.leak-button:hover {
  background: #cc3333;
}

.leak-button:active {
  background: #aa2222;
}

.restart-button {
  background: #ffa500;
}

.restart-button:hover {
  background: #cc8400;
}

.restart-button:active {
  background: #aa6d00;
}

.crash-button {
  background: #9400d3;
}

.crash-button:hover {
  background: #7600a9;
}

.crash-button:active {
  background: #5a0080;
}

.chart-container {
  background: #1a1a1a;
  border-radius: 8px;
  padding: 1rem;
  height: 300px;
  width: 100%;
}
</style>
