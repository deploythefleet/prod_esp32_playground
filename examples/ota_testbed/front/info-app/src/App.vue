<script setup>
import { ref, onMounted, computed } from 'vue'
import MemoryChart from './components/MemoryChart.vue'
import { useUptime } from './composables/useUptime'

const systemInfo = ref(null)
const loading = ref(true)
const error = ref(null)

const uptimeMs = computed(() => systemInfo.value?.uptime_ms || 0)
const formattedUptime = useUptime(uptimeMs)

const fetchSystemInfo = async () => {
  try {
    const response = await fetch('/api/v1/system/info')
    if (!response.ok) {
      throw new Error('Failed to fetch system info')
    }
    systemInfo.value = await response.json()
  } catch (err) {
    error.value = err.message
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  fetchSystemInfo()
})
</script>

<template>
  <div class="container">
    <h1>ESP32 Info</h1>
    
    <div v-if="loading" class="status">Loading...</div>
    <div v-else-if="error" class="status error">Error: {{ error }}</div>
    <div v-else-if="systemInfo" class="info-card">
      <div class="info-item">
        <span class="label">IDF Version:</span>
        <span class="value">{{ systemInfo.version }}</span>
      </div>
      <div class="info-item">
        <span class="label">CPU Cores:</span>
        <span class="value">{{ systemInfo.cores }}</span>
      </div>
      <div class="info-item">
        <span class="label">App Version:</span>
        <span class="value">{{ systemInfo.app_version }}</span>
      </div>
      <div class="info-item">
        <span class="label">Build Date(UTC):</span>
        <span class="value">{{ systemInfo.build_date }} {{ systemInfo.build_time }}</span>
      </div>
      <div class="info-item">
        <span class="label">Uptime:</span>
        <span class="value">{{ formattedUptime }}</span>
      </div>
      <div class="info-item">
        <span class="label">Reset reason:</span>
        <span class="value">{{ systemInfo.reset_reason }}</span>
      </div>
    </div>
    
    <div v-if="systemInfo" class="refresh-container">
      <button @click="fetchSystemInfo" class="refresh-button" title="Refresh system info">
        ↻
      </button>
    </div>

    <MemoryChart v-if="systemInfo" />
  </div>
</template>

<style scoped>
.container {
  max-width: 900px;
  margin: 0 auto;
  padding: 2rem;
}

h1 {
  color: #42b883;
  margin-bottom: 1rem;
  text-align: center;
}

.version-indicator {
  text-align: center;
  font-size: 0.75rem;
  color: #666;
  margin-bottom: 1rem;
}

.status {
  text-align: center;
  padding: 1rem;
  color: #888;
}

.status.error {
  color: #ff4444;
}

.info-card {
  background: #1a1a1a;
  border-radius: 8px;
  padding: 1.5rem;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  max-width: 600px;
  margin: 0 auto;
}

.info-item {
  display: flex;
  justify-content: space-between;
  padding: 0.75rem 0;
  border-bottom: 1px solid #333;
}

.info-item:last-child {
  border-bottom: none;
}

.label {
  font-weight: 600;
  color: #aaa;
}

.value {
  color: #fff;
  font-family: monospace;
}

.refresh-container {
  text-align: center;
  margin-top: 1rem;
}

.refresh-button {
  background: #42b883;
  color: white;
  border: none;
  padding: 0.5rem 1rem;
  border-radius: 4px;
  font-size: 1rem;
  cursor: pointer;
  transition: background 0.2s;
}

.refresh-button:hover {
  background: #35a372;
}

.refresh-button:active {
  background: #2d8a5f;
}
</style>
