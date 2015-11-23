--[[
 *
 *
 *  Copyright (C) Nogs GmbH, Andre Riesberg
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, 
 *  write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
--]]

-- NOGS CN Lua bootstrap loader

net.ip = '192.168.1.249'
net.mask = '255.255.255.0'
 
collectgarbage()
sys.print(sys.peek(1),'Bootloader ready. IP:',net.ip,'Mask:',net.mask," Used memory",collectgarbage('count'))

sys.execute = function()
  collectgarbage()
  -- sys.print('*** sys.execute --',sys.source)
  local app, error = load(sys.source)
  sys.source = nil
  collectgarbage()
  if app then
    local success, result = pcall(app)
    if success then
      sys.print('Success',result)
      if result == nil then
        return 'nil'
      else 
        return ''..result
      end
    else
      sys.print('Error during pcall',result)
      return '[{"error":"'..string.gsub(result,'%"',' ')..'"}]'
    end
  else
    sys.print('Error during load:',error)
    return '[{"error":"'..string.gsub(error,'%"',' ')..'"}]'
  end
end

net.ethInit(net.ipToNum(net.ip),net.ipToNum(net.mask))

net.onUDPRecvFunction = {}

net.onUDPRecv = function(port)
  sys.print('UDP',port)
  if net.onUDPRecvFunction[port] then
    return net.onUDPRecvFunction[port](net.getPacketAsString())
  end
end

net.onUDPRecvFunction[20144] = function(packet)
  -- sys.print('Packet: [',packet,']')
  sys.source = packet
  packet = nil
  return sys.execute()
end

sys.onIdle = function()
end

while true do
  sys.onIdle()
end
