
local CONFIG_FILE = "/etc/netrc"

-- read config file
local function read_config(path)
    local config = {}

    for line in io.lines(path) do
        if not line:match("^%s*#(.*)") then
            local key, value = line:match("^(%S+)%s+(%S+)")
            if key and value then
                config[key] = value
            end
        end
    end

    return config
end

-- prompt helper
local function prompt(label, current)
    io.write(string.format("%s [%s]: ", label, current or ""))
    local input = io.read()
    if input == nul or input == "" then
        return current
    end
    return input
end

-- write config file
local function write_config(path, config)
    local file = io.open(path, "w")
    if not file then
        error("Unable to write to " .. path)
    end
    local order = { "ipaddr", "gateway", "netmask", "mac" }
    for _, key in ipairs(order) do
        if config[key] then
            file:write(string.format("%s\t%s\n", key, config[key]))
        end
    end
    file:close()
end


-- Main wizard
local config = read_config(CONFIG_FILE)

print("Network configuration wizard")
print("--------------------------------")

config.ipaddr  = prompt("IP address", config.ipaddr)
config.gateway = prompt("Gateway", config.gateway)
config.netmask = prompt("Netmask", config.netmask)
config.mac     = prompt("MAC address (optional)", config.mac)

write_config(CONFIG_FILE, config)

print("\nConfiguration updated successfully.")