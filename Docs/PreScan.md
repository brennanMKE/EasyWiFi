# WiFi Pre-Scan Feature

**Date Added**: December 14, 2024  
**Version**: 0.1.0  
**Feature**: Intelligent Network Range Filtering

---

## Overview

The pre-scan feature scans for available WiFi networks before attempting to connect, filtering out networks that are not in range. This significantly reduces connection time when multiple networks are stored but only some are available.

## How It Works

### 1. Pre-Scan Process

When multiple networks are stored (2+), the system performs a quick scan before connection attempts:

```cpp
WiFi.mode(WIFI_STA);
int numScanned = WiFi.scanNetworks(false, false);  // Quick scan, no hidden SSIDs
```

### 2. 2.4GHz Filtering

The scan only considers 2.4GHz networks (channels 1-14), since the ESP32-C3 doesn't support 5GHz:

```cpp
if (channel >= 1 && channel <= 14) {
    availableSSIDs.push_back(scannedSSID);
}
```

### 3. Network Matching

Each stored credential is checked against the scan results:

```cpp
for (size_t i = 0; i < credentials.size(); i++) {
    networkInRange[i] = false;
    for (const auto& available : availableSSIDs) {
        if (available == credentials[i].ssid) {
            networkInRange[i] = true;
            break;
        }
    }
}
```

### 4. Selective Connection

- **Phase 1**: Only attempts if first network is in range
- **Phase 2**: Only adds in-range networks to WiFiMulti
- **All Out of Range**: Returns `SSID_NOT_FOUND` immediately without connection attempts

## Benefits

### Time Savings

**Without Pre-Scan**:
- 5 stored networks, 2 out of range
- Each failed attempt: 30 seconds timeout
- Total wasted time: 60 seconds

**With Pre-Scan**:
- Scan time: 3-5 seconds
- Only tries 3 in-range networks
- Saves: ~55 seconds

### Resource Savings

- Reduces WiFi radio time (power consumption)
- Fewer watchdog resets needed
- Less log clutter from failed attempts

### Better User Experience

- Faster transition to AP mode when no networks available
- Clear logging shows which networks are in/out of range
- More predictable behavior

## Configuration

The feature is controlled by a macro in `Macros.h`:

```cpp
#define WIFI_PRE_SCAN_ENABLED true  // Set to false to disable
```

### When to Disable

You might want to disable pre-scan if:

1. **Only one network stored**: Pre-scan adds overhead for single-network setups
2. **Hidden SSIDs**: Pre-scan won't detect hidden networks
3. **Extremely stable environment**: If networks never change, the scan is unnecessary

## Logging Output

### With Networks In Range

```
[WIFI_MGR] ========================================
[WIFI_MGR] Pre-Scan: Checking which networks are in range
[WIFI_MGR] ========================================
[WIFI_MGR] Found 15 network(s) in range
[WIFI_MGR] Found 12 2.4GHz network(s) in range
[WIFI_MGR]   Winterfell: ✓ In range
[WIFI_MGR]   MayorDanielLurie: ✓ In range
[WIFI_MGR]   HomeNetwork: ✗ Out of range
```

### All Networks Out of Range

```
[WIFI_MGR] Phase 2: SKIPPED - No remaining networks in range
[WIFI_MGR] All 3 stored network(s) are out of range
[WIFI_MGR] ========================================
```

### Phase Skipping

```
[WIFI_MGR] Phase 1: SKIPPED - Network 1 (Winterfell) is out of range
[WIFI_MGR] ========================================
```

## Implementation Details

### Scan Characteristics

- **Type**: Active scan (sends probe requests)
- **Hidden Networks**: Not detected (`scanNetworks(false, false)`)
- **Duration**: Typically 3-5 seconds
- **Cleanup**: `WiFi.scanDelete()` called after processing

### Network Tracking

A boolean vector tracks in-range status:

```cpp
std::vector<bool> networkInRange(credentials.size(), true);
```

- **Default**: Assumes all in range (fallback if scan fails)
- **Updated**: Set to true/false based on scan results
- **Used**: Throughout Phase 1 and Phase 2 to skip unreachable networks

### Error Handling

If the scan fails (`numScanned <= 0`):
- Logs warning: "Pre-scan found no networks or failed"
- Falls back to trying all stored networks
- Ensures the feature doesn't break connectivity in edge cases

## Performance Impact

### Scan Time

| Environment | Networks Found | Scan Duration |
|------------|----------------|---------------|
| Urban      | 15-30 networks | 4-6 seconds   |
| Suburban   | 5-15 networks  | 3-5 seconds   |
| Rural      | 1-5 networks   | 2-4 seconds   |

### Memory Usage

- **RAM**: ~100 bytes per scanned network (temporary)
- **Stack**: ~200 bytes for tracking vectors
- **Cleanup**: All scan data freed after use

## Integration with Existing Features

### With eero Compatibility

Pre-scan doesn't affect the Phase 1 eero compatibility settings:

1. Scan determines if network is in range
2. If in range, Phase 1 applies eero settings (low power, 802.11b/g)
3. If out of range, Phase 1 is skipped entirely

### With Multi-Network Priority

The feature respects the credential order:

1. First network gets Phase 1 attempt (if in range)
2. Remaining in-range networks get Phase 2 attempt (if first fails)
3. Most recently connected network moves to first position

### With Error Handling

Pre-scan integrates with the error handler:

```cpp
WiFiError error = WiFiError::SSID_NOT_FOUND;
errorHandler.recordError((uint32_t)error, "All networks out of range");
```

## Special Cases

### Single Network

If only one network is stored:
- Pre-scan is still performed (if enabled)
- Phase 2 is skipped (no remaining networks)
- Returns error if network is out of range

### Hidden Networks

Hidden SSIDs won't appear in scan results:
- Will be marked as "out of range"
- Won't be attempted
- **Solution**: Disable pre-scan or make SSID visible

### Intermittent Signals

If a network disappears between scan and connection:
- Scan shows it in range
- Connection attempt proceeds normally
- Times out after 30 seconds
- No different than without pre-scan

## Future Enhancements

### Possible Improvements

1. **Cache Scan Results**: Valid for 30-60 seconds
2. **Signal Strength Sorting**: Try strongest networks first
3. **Hidden Network Support**: Special handling for hidden SSIDs
4. **Adaptive Timeout**: Shorter timeout for weak signals
5. **Background Scanning**: Update available networks while connected

## Testing Checklist

When testing the pre-scan feature:

- [ ] Multiple networks stored, all in range
- [ ] Multiple networks stored, some out of range
- [ ] Multiple networks stored, all out of range
- [ ] Single network stored, in range
- [ ] Single network stored, out of range
- [ ] Hidden SSID (should require disabling pre-scan)
- [ ] Scan failure scenario (should fall back gracefully)
- [ ] Network appears/disappears between scan and connection

## Configuration Example

### Enable Pre-Scan (Default)

```cpp
// lib/EasyWiFi/Macros.h
#define WIFI_PRE_SCAN_ENABLED true
```

### Disable Pre-Scan

```cpp
// lib/EasyWiFi/Macros.h
#define WIFI_PRE_SCAN_ENABLED false
```

## Troubleshooting

### "Pre-scan found no networks or failed"

**Possible Causes**:
- WiFi radio not initialized properly
- Antenna issue
- Environmental interference
- Scan timeout

**Solution**: The system falls back to trying all networks.

### "Network shows in scan but connection fails"

**Possible Causes**:
- Signal too weak for connection (but strong enough for detection)
- Network disappeared between scan and connection
- Authentication issues

**Solution**: Normal connection timeout and retry logic applies.

### "Hidden network not connecting"

**Cause**: Hidden SSIDs don't appear in scan results.

**Solution**: 
1. Set `#define WIFI_PRE_SCAN_ENABLED false` in Macros.h
2. Or make the SSID visible

## Conclusion

The pre-scan feature is a smart optimization that:

✅ Reduces connection time in multi-network scenarios  
✅ Saves power by avoiding doomed connection attempts  
✅ Provides better logging and diagnostics  
✅ Gracefully degrades if scanning fails  
✅ Respects existing eero compatibility and priority features

**Recommendation**: Leave enabled unless you have hidden SSIDs or a single-network setup where the 3-5 second scan overhead matters.

