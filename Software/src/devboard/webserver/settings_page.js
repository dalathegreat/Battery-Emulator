

    function askFactoryReset() {
      if (confirm('Are you sure you want to reset the device to factory settings? This will erase all settings and data.')) {
        var xhr = new XMLHttpRequest();
        xhr.onload = function() {
          if (this.status == 200) {
            alert('Factory reset successful. The device will now restart.');
            reboot();
          } else {
            alert('Factory reset failed. Please try again.');
          }
        };
        xhr.onerror = function() {
          alert('An error occurred while trying to reset the device.');
        };
        xhr.open('POST', '/factoryReset', true);
        xhr.send();
      }
    }

    function editComplete(){if(this.status==200){window.location.reload();}}

    function editError(){alert('Invalid input');}
        function editRecoveryMode(){var value=prompt('Extremely dangerous option. Emergency charge allows recovery for a severely undercharged battery. Limit charge power to avoid cell rupture and possible fire. Start 30min recovery process? (0 = No, 1 = Yes):');
          if(value!==null){if(value==0||value==1){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/enableRecoveryMode?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1.');}}}

        function editWh(){var value=prompt('How much energy the battery can store. Enter new Wh value (1-400000):');
          if(value!==null){if(value>=1&&value<=400000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateBatterySize?value='+value,true);xhr.send();}else{
          alert('Invalid value. Please enter a value between 1 and 400000.');}}}

        function editUseScaledSOC(){var value=prompt('Extends battery life by rescaling the SOC within the configured minimum and maximum percentage. Should SOC scaling be applied? (0 = No, 1 = Yes):');
          if(value!==null){if(value==0||value==1){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateUseScaledSOC?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1.');}}}
    
        function editSocMax(){var value=prompt('Inverter will see fully charged (100pct)SOC when this value is reached. Enter new maximum SOC value that battery will charge to (50.0-100.0):');if(value!==null){if(value>=50&&value<=100){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateSocMax?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 50.0 and 100.0');}}}
    


        function editSocMin(){
          var value=prompt('Inverter will see completely discharged (0pct)SOC when this value is reached. Advanced users can set to negative values. Enter new minimum SOC value that battery will discharge to (-10.0to50.0):');
          if(value!==null){if(value>=-10&&value<=50){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateSocMin?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between -10 and 50.0');}}}
    
        function editMaxChargeA(){var value=prompt('Some inverters needs to be artificially limited. Enter new maximum charge current in A (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateMaxChargeA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000.0');}}}
    
        function editMaxDischargeA(){var value=prompt('Some inverters needs to be artificially limited. Enter new maximum discharge current in A (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateMaxDischargeA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000.0');}}}
    
        function editUseVoltageLimit(){var value=prompt('Enable this option to manually restrict charge/discharge to a specific voltage set below. If disabled the emulator automatically determines this based on battery limits. Restrict manually? (0 = No, 1 = Yes):');if(value!==null){if(value==0||value==1){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateUseVoltageLimit?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1.');}}}
    
        function editMaxChargeVoltage(){var value=prompt('Some inverters needs to be artificially limited. Enter new voltage setpoint batttery should charge to (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var 
        xhr=new XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateMaxChargeVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000.0');}}}
    
        function editMaxDischargeVoltage(){var value=prompt('Some inverters needs to be artificially limited. Enter new voltage setpoint batttery should discharge to (0-1000.0):');if(value!==null){if(value>=0&&value<=1000){var 
        xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateMaxDischargeVoltage?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 1000.0');}}}

        function editBMSresetDuration(){var value=prompt('Amount of seconds BMS power pin should be low during periodic resets. Requires "Periodic BMS Power reset" to be enabled. Enter value in seconds (1-600):');if(value!==null){if(value>=1&&value<=600){var 
        xhr=new XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateBMSresetDuration?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 1 and 600.');}}}

        function startBMSReset(){if(confirm('Cut BMS power now? Charging and discharging are paused until it is back up.')){var xhr=new XMLHttpRequest();
        xhr.onload=function(){alert(this.status==200?'BMS Power reset started.':this.responseText);};xhr.onerror=editError;xhr.open('POST','/startBMSReset',true);xhr.send();}}


        function calibrateQnhZero(){if(confirm('No current may flow through the sensor while it is measured: open the contactors, or take the clamp off the cable, and wait a few seconds.\n\nMeasure its zero point now?')){var xhr=new XMLHttpRequest();
        xhr.onload=function(){if(this.status==200){document.getElementById('qnhzero').textContent=this.responseText;}alert(this.status==200?'Zero point set to '+this.responseText+'.':this.responseText);};xhr.onerror=editError;xhr.open('POST','/calibrateShuntZero',true);xhr.send();}}
        function editTeslaBalAct(){var value=prompt('Enable or disable forced LFP balancing. Makes the battery charge to 101percent. This should be performed once every month, to keep LFP batteries balanced. Ensure battery is fully charged before enabling, and also that you have enough sun or grid power to feed power into the battery while balancing is active. Enter 1 for enabled, 0 for disabled');if(value!==null){if(value==0||value==1){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/TeslaBalAct?value='+value,true);xhr.send();}}else{alert('Invalid value. Please enter 1 or 0');}}
    
        function editBalTime(){var value=prompt('Enter new max balancing time in minutes');if(value!==null){if(value>=1&&value<=300){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalTime?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 1 and 300');}}}
    
        function editBalFloatPower(){var value=prompt('Power level in Watt to float charge during forced balancing');if(value!==null){if(value>=100&&value<=2000){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalFloatPower?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 100 and 2000');}}}
    
        function editBalMaxPackV(){var value=prompt('Battery pack max voltage temporarily raised to this value during forced balancing. Value in V');if(value!==null){if(value>=380&&value<=410){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalMaxPackV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 380 and 410');}}}

        function editBalMaxCellV(){var value=prompt('Cellvoltage max temporarily raised to this value during forced balancing. Value in mV');if(value!==null){if(value>=3400&&value<=3750){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalMaxCellV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 3400 and 3750');}}}
    
        function editBalMaxDevCellV(){var value=prompt('Cellvoltage max deviation temporarily raised to this value during forced balancing. Value in mV');if(value!==null){if(value>=300&&value<=600){var xhr=new 
        XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/BalMaxDevCellV?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 300 and 600');}}}

          function editChargerHVDCEnabled(){var value=prompt('Enable or disable HV DC output. Enter 1 for enabled, 0 for disabled');if(value!==null){if(value==0||value==1){var xhr=new 
          XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargerHvEnabled?value='+value,true);xhr.send();}}else{alert('Invalid value. Please enter 1 or 0');}}

          function editChargerAux12vEnabled(){var value=prompt('Enable or disable low voltage 12v auxiliary DC output. Enter 1 for enabled, 0 for disabled');if(value!==null){if(value==0||value==1){var xhr=new 
          XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargerAux12vEnabled?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter 1 or 0');}}}

          function editChargerSetpointVDC(){var value=prompt('Set charging voltage. Input will be validated against inverter and/or charger configuration parameters, but use sensible values like 200 to 420.');
            if(value!==null){if(value>=0&&value<=1000){var xhr=new XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargeSetpointV?value='+value,true);xhr.send();}else{
            alert('Invalid value. Please enter a value between 0 and 1000');}}}

          function editChargerSetpointIDC(){var value=prompt('Set charging amperage. Input will be validated against inverter and/or charger configuration parameters, but use sensible values like 6 to 48.');
            if(value!==null){if(value>=0&&value<=1000){var xhr=new           XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargeSetpointA?value='+value,true);xhr.send();}else{
              alert('Invalid value. Please enter a value between 0 and 100');}}}

          function editChargerSetpointEndI(){
            var value=prompt('Set amperage that terminates charge as being sufficiently complete. Input will be validated against inverter and/or charger configuration parameters, but use sensible values like 1-5.');
            if(value!==null){if(value>=0&&value<=1000){var xhr=new 
          XMLHttpRequest();xhr.onload=editComplete;xhr.onerror=editError;xhr.open('GET','/updateChargeEndA?value='+value,true);xhr.send();}else{alert('Invalid value. Please enter a value between 0 and 100');}}}

          function goToMainPage() { window.location.href = '/'; }

          function haDisc(c) {
            var f = document.querySelector("[name=HADISCFWU]"), n = document.querySelector("[name=HADISC]");
            if (!c.checked) { f.checked = n.checked = false; } else if (!f.checked && !n.checked) { f.checked = true; }
          }

          document.querySelectorAll('select,input').forEach(function(sel) {
            function ch() {
              sel.closest('form').setAttribute('data-' + sel.name?.toLowerCase(), sel.type=='checkbox'?sel.checked:sel.value);
            }
            sel.addEventListener('change', ch);
            ch();
          });

          // A shunt type hidden for the selected battery/inverter must not stay selected
          function syncShunt() {
            var s = document.querySelector('select[name=shunttype]'), o = s && s.options[s.selectedIndex];
            if (o && getComputedStyle(o).display == 'none') { s.value = '0'; s.dispatchEvent(new Event('change')); }
          }
          document.querySelectorAll('select[name=battery],select[name=inverter]').forEach(function(sel) {
            sel.addEventListener('change', syncShunt);
          });
          syncShunt();

          var iu=document.getElementById('invutc'),ie=iu?+iu.textContent:0;
          if(ie>0&&ie<4e12){iu.textContent=new Date(ie*1000).toISOString().replace('T',' ').slice(0,19);}
    