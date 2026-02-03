import { computed } from 'vue'

/**
 * Composable to format uptime in milliseconds to a human-readable string
 * - Below 60s: shows in seconds
 * - Over 1 minute: shows in minutes and seconds
 * - Over 1 hour: shows in hours and minutes
 * - Over 1 day: shows in days and hours
 * 
 * @param {Ref<number>|number} uptimeMs - Uptime in milliseconds
 * @returns {ComputedRef<string>} Formatted uptime string
 */
export function useUptime(uptimeMs) {
  const formattedUptime = computed(() => {
    const totalSeconds = Math.floor(uptimeMs.value / 1000)
    
    if (totalSeconds < 60) {
      return `${totalSeconds}s`
    }
    
    const totalMinutes = Math.floor(totalSeconds / 60)
    const seconds = totalSeconds % 60
    
    if (totalSeconds < 3600) {
      return `${totalMinutes}m ${seconds}s`
    }
    
    const totalHours = Math.floor(totalSeconds / 3600)
    const minutes = totalMinutes % 60
    
    if (totalSeconds < 86400) {
      return `${totalHours}h ${minutes}m`
    }
    
    const days = Math.floor(totalSeconds / 86400)
    const hours = totalHours % 24
    
    return `${days}d ${hours}h`
  })
  
  return formattedUptime
}
