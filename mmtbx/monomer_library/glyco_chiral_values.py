from __future__ import division
import os, sys

volumes = {
  '0AT' :  -2.4,
  '0MK' :  -2.4,
  '0NZ' :   2.4,
  '0XY' :  -2.4,
  '0YT' :   3.1,
  '16G' :  -2.4,
  '1AR' :  -2.4,
  '1GL' :  -2.4,
  '1GN' :   2.4,
  '1NA' :   2.4,
  '1S3' :  -2.4,
  '27C' :  -2.5,
  '289' :  -2.4,
  '291' :  -2.4,
  '293' :  -2.4,
  '2DG' :  -2.4,
  '2F8' :  -2.4,
  '2FG' :   2.4,
  '2GL' :   2.4,
  '2M5' :  -2.4,
  '32O' :  -2.5,
  '34V' :   2.7,
  '3DO' :   2.4,
  '3DY' :  -2.4,
  '3FM' :  -2.4,
  '3MG' :   2.4,
  '46M' :   2.4,
  '48Z' :  -2.5,
  '50A' :  -2.5,
  '5GF' :   2.4,
  '7JZ' :   2.4,
  'A1Q' :  -2.4,
  'A2G' :  -2.4,
  'AAL' :   2.4,
  'ABE' :  -2.4,
  'ABF' :   2.5,
  'ADA' :  -2.4,
  'ADG' :  -2.4,
  'AFD' :  -2.4,
  'AFL' :  -2.4,
  'AFO' :  -2.5,
  'AFP' :   2.7,
  'AGC' :  -2.4,
  'AGH' :  -2.4,
  'AGL' :  -2.4,
  'AHR' :   2.5,
  'AIG' :   2.4,
  'ALL' :   2.4,
  'AMU' :   2.4,
  'AOG' :   2.4,
  'ARA' :   2.4,
  'ARB' :  -2.4,
  'ARI' :   2.4,
  'ASG' :   2.4,
  'AXR' :  -2.6,
  'B16' :   2.4,
  'B6D' :   2.4,
  'B8D' :  -2.4,
  'B9D' :   2.4,
  'BBK' :  -3.1,
  'BDG' :  -2.4,
  'BDP' :   2.4,
  'BDR' :   2.5,
  'BEM' :   2.4,
  'BFP' :  -2.6,
  'BGC' :   2.4,
  'BGL' :   2.4,
  'BGP' :   2.4,
  'BGS' :   3.0,
  'BHG' :   2.4,
  'BMA' :   2.4,
  'BMX' :  -2.4,
  'BNG' :   2.4,
  'BOG' :   2.4,
  'BRI' :  -2.4,
  'BXF' :   2.4,
  'BXX' :   2.5,
  'BXY' :  -2.6,
  'CDR' :   2.4,
  'CEG' :   2.4,
  'D6G' :  -2.4,
  'DAG' :   2.4,
  'DDA' :   2.4,
  'DDB' :   2.4,
  'DDL' :   2.4,
  'DFR' :  -2.7,
  'DGC' :   2.4,
  'DGS' :  -2.4,
  'DLF' :   2.4,
  'DLG' :   2.4,
  'DR4' :   2.4,
  'DRI' :   2.4,
  'DSR' :   2.4,
  'DT6' :   2.4,
  'DVC' :   2.4,
  'E5G' :  -2.4,
  'EAG' :   2.4,
  'EGA' :   2.4,
  'ERE' :   2.4,
  'ERI' :   2.4,
  'F1P' :  -2.6,
  'F1X' :  -2.6,
  'F6P' :  -2.6,
  'FBP' :  -2.6,
  'FCA' :  -2.4,
  'FCB' :   2.4,
  'FDP' :  -2.6,
  'FRU' :  -2.6,
  'FUB' :  -2.5,
  'FUC' :   2.4,
  'FUL' :  -2.4,
  'G16' :  -2.4,
  'G1P' :  -2.4,
  'G2F' :  -2.4,
  'G4D' :  -2.4,
  'G4S' :   2.3,
  'G6D' :  -2.4,
  'G6P' :  -2.4,
  'G6S' :   2.4,
  'GAL' :   2.4,
  'GC4' :   2.4,
  'GCD' :   2.4,
  'GCN' :  -2.4,
  'GCS' :   2.4,
  'GCU' :  -2.4,
  'GCV' :  -2.4,
  'GCW' :   2.4,
  'GE1' :  -2.4,
  'GFP' :  -2.4,
  'GIV' :  -2.4,
  'GL0' :   2.4,
  'GLA' :  -2.4,
  'GLB' :   2.4,
  'GLC' :  -2.4,
  'GLD' :  -2.4,
  'GLP' :  -2.4,
  'GLT' :  -3.1,
  'GLW' :  -2.4,
  'GMH' :  -2.4,
  'GN1' :  -2.3,
  'GNX' :  -2.4,
  'GP1' :  -2.4,
  'GP4' :  -2.4,
  'GPH' :   2.4,
  'GQ1' :  -2.4,
  'GS1' :   3.0,
  'GS4' :   2.4,
  'GSA' :   2.4,
  'GSD' :   2.4,
  'GTK' :   2.5,
  'GTR' :   2.4,
  'GU0' :   2.4,
  'GU1' :   2.4,
  'GU2' :   2.3,
  'GU3' :  -2.4,
  'GU4' :  -2.4,
  'GU5' :  -2.4,
  'GU6' :  -2.4,
  'GU8' :   2.4,
  'GU9' :  -2.4,
  'GUF' :   2.4,
  'GUP' :   2.4,
  'GUZ' :  -2.4,
  'GYP' :  -2.4,
  'H2P' :   2.6,
  'HSG' :   2.4,
  'HSH' :   2.4,
  'HSJ' :  -2.4,
  'HSQ' :   2.4,
  'HSR' :   2.4,
  'HSU' :   2.6,
  'HSX' :  -2.5,
  'HSY' :   2.4,
  'HSZ' :   2.4,
  'IDG' :  -2.4,
  'IDR' :   2.4,
  'IDS' :   2.4,
  'IDT' :   2.4,
  'IDU' :  -2.4,
  'IDX' :   2.4,
  'IDY' :   2.4,
  'IN1' :   2.4,
  'IPT' :   3.0,
  'ISL' :  -2.4,
  'KBG' :   2.4,
  'KDM' :   2.5,
  'L6S' :   2.4,
  'LDY' :  -2.4,
  'LGU' :   2.4,
  'LVZ' :  -2.4,
  'LXB' :   2.4,
  'LXZ' :  -2.4,
  'M6P' :  -2.4,
  'M8C' :  -2.3,
  'MA1' :  -3.0,
  'MA2' :  -2.4,
  'MA3' :  -2.4,
  'MAG' :   2.4,
  'MAN' :  -2.4,
  'MAT' :   2.4,
  'MAV' :  -2.4,
  'MAW' :   2.4,
  'MBG' :   2.4,
  'MCU' :   2.4,
  'MDA' :   2.4,
  'MDP' :   2.4,
  'MFA' :   2.4,
  'MFB' :  -2.3,
  'MFU' :   2.4,
  'MG5' :   2.4,
  'MGA' :   2.4,
  'MGL' :   2.4,
  'MMA' :  -2.4,
  'MRP' :   2.4,
  'MXY' :  -2.4,
  'N1L' :   2.4,
  'NAA' :   2.4,
  'NAG' :   2.4,
  'NDG' :  -2.4,
  'NED' :  -2.4,
  'NG1' :  -2.4,
  'NG6' :   2.4,
  'NGA' :   2.4,
  'NGC' :   2.5,
  'NGE' :  -2.5,
  'NGL' :   2.4,
  'NGS' :   2.4,
  'NGY' :  -2.4,
  'NM6' :   2.4,
  'NM9' :   2.4,
  'OPM' :  -2.4,
  'ORP' :  -2.5,
  'P6P' :   2.7,
  'PRP' :  -2.5,
  'PSV' :   2.6,
  'R1P' :  -2.5,
  'RAA' :   2.4,
  'RAE' :   2.4,
  'RAM' :   2.4,
  'RAO' :   2.4,
  'RDP' :  -2.5,
  'RER' :   2.4,
  'RF5' :  -2.5,
  'RG1' :  -2.4,
  'RGG' :   2.4,
  'RHA' :   2.4,
  'RIB' :  -2.5,
  'RIP' :   2.4,
  'RPA' :   2.4,
  'RST' :   2.4,
  'RUU' :  -2.6,
  'SF6' :  -2.6,
  'SF9' :   2.7,
  'SG4' :  -2.4,
  'SG5' :  -2.4,
  'SG6' :  -2.4,
  'SG7' :  -2.4,
  'SGA' :   2.4,
  'SGC' :   2.4,
  'SGD' :  -2.4,
  'SGN' :  -2.4,
  'SHB' :   2.4,
  'SHG' :   2.4,
  'SOE' :  -2.6,
  'SSG' :   3.0,
  'SUS' :  -2.2,
  'T6T' :   2.6,
  'TM9' :   2.4,
  'TMR' :   2.4,
  'TMX' :   2.4,
  'TOA' :  -2.4,
  'TOC' :  -2.4,
  'TYV' :  -2.4,
  'UDC' :  -2.4,
  'X1X' :  -2.4,
  'X2F' :  -2.4,
  'X6X' :  -2.4,
  'XLF' :  -2.6,
  'XXR' :  -2.4,
  'XYP' :   2.4,
  'XYS' :  -2.4,
  'Z6J' :   2.5,
  'Z9M' :   2.4,
}
alpha_beta = {
  '0AT' : 'alpha',
  '0MK' : 'beta',
  '0NZ' : 'beta',
  '0XY' : 'alpha',
  '0YT' : 'beta',
  '16G' : 'alpha',
  '1AR' : 'beta',
  '1GL' : 'alpha',
  '1GN' : 'beta',
  '1NA' : 'beta',
  '1S3' : 'beta',
  '27C' : 'alpha',
  '289' : 'alpha',
  '291' : 'alpha',
  '293' : 'alpha',
  '2DG' : 'alpha',
  '2F8' : 'alpha',
  '2FG' : 'beta',
  '2GL' : 'beta',
  '2M5' : 'alpha',
  '32O' : 'beta',
  '34V' : 'beta',
  '3FM' : 'alpha',
  '3MG' : 'beta',
  '46M' : 'beta',
  '48Z' : 'alpha',
  '50A' : 'alpha',
  '5GF' : 'beta',
  '7JZ' : 'beta',
  'A1Q' : 'alpha',
  'A2G' : 'alpha',
  'AAL' : 'alpha',
  'ABE' : 'alpha',
  'ABF' : 'beta',
  'ADA' : 'beta',
  'ADG' : 'alpha',
  'AFD' : 'alpha',
  'AFL' : 'beta',
  'AFO' : 'alpha',
  'AFP' : 'alpha',
  'AGC' : 'alpha',
  'AGH' : 'alpha',
  'AGL' : 'alpha',
  'AHR' : 'alpha',
  'AIG' : 'beta',
  'ALL' : 'beta',
  'AMU' : 'beta',
  'AOG' : 'beta',
  'ARA' : 'alpha',
  'ARB' : 'beta',
  'ARI' : 'beta',
  'ASG' : 'beta',
  'AXR' : 'alpha',
  'B16' : 'alpha',
  'B6D' : 'beta',
  'B8D' : 'alpha',
  'B9D' : 'alpha',
  'BBK' : 'alpha',
  'BDG' : 'alpha',
  'BDP' : 'alpha',
  'BDR' : 'beta',
  'BEM' : 'alpha',
  'BFP' : 'beta',
  'BGC' : 'beta',
  'BGL' : 'beta',
  'BGP' : 'beta',
  'BGS' : 'alpha',
  'BHG' : 'beta',
  'BMA' : 'beta',
  'BMX' : 'alpha',
  'BNG' : 'beta',
  'BOG' : 'beta',
  'BRI' : 'alpha',
  'BXF' : 'beta',
  'BXX' : 'beta',
  'BXY' : 'alpha',
  'CDR' : 'beta',
  'CEG' : 'beta',
  'D6G' : 'alpha',
  'DAG' : 'beta',
  'DDA' : 'beta',
  'DDB' : 'beta',
  'DDL' : 'beta',
  'DFR' : 'alpha',
  'DGC' : 'alpha',
  'DGS' : 'alpha',
  'DLF' : 'alpha',
  'DLG' : 'beta',
  'DR4' : 'beta',
  'DRI' : 'beta',
  'DSR' : 'beta',
  'DT6' : 'alpha',
  'DVC' : 'alpha',
  'E5G' : 'alpha',
  'EAG' : 'beta',
  'EGA' : 'beta',
  'ERE' : 'alpha',
  'ERI' : 'alpha',
  'F1P' : 'beta',
  'F1X' : 'beta',
  'F6P' : 'beta',
  'FBP' : 'beta',
  'FCA' : 'alpha',
  'FCB' : 'beta',
  'FDP' : 'alpha',
  'FRU' : 'beta',
  'FUB' : 'beta',
  'FUC' : 'alpha',
  'FUL' : 'beta',
  'G16' : 'beta',
  'G1P' : 'beta',
  'G2F' : 'alpha',
  'G4D' : 'beta',
  'G4S' : 'beta',
  'G6D' : 'alpha',
  'G6P' : 'alpha',
  'G6S' : 'beta',
  'GAL' : 'beta',
  'GC4' : 'alpha',
  'GCD' : 'alpha',
  'GCN' : 'alpha',
  'GCS' : 'beta',
  'GCU' : 'beta',
  'GCV' : 'beta',
  'GCW' : 'alpha',
  'GE1' : 'beta',
  'GFP' : 'beta',
  'GIV' : 'beta',
  'GL0' : 'beta',
  'GLA' : 'alpha',
  'GLB' : 'beta',
  'GLC' : 'alpha',
  'GLD' : 'alpha',
  'GLP' : 'alpha',
  'GLT' : 'alpha',
  'GLW' : 'alpha',
  'GMH' : 'alpha',
  'GN1' : 'beta',
  'GNX' : 'alpha',
  'GP1' : 'beta',
  'GP4' : 'alpha',
  'GPH' : 'beta',
  'GQ1' : 'alpha',
  'GS1' : 'beta',
  'GS4' : 'beta',
  'GSA' : 'beta',
  'GSD' : 'beta',
  'GTK' : 'beta',
  'GTR' : 'alpha',
  'GU0' : 'beta',
  'GU1' : 'alpha',
  'GU2' : 'beta',
  'GU3' : 'alpha',
  'GU4' : 'alpha',
  'GU5' : 'alpha',
  'GU6' : 'beta',
  'GU8' : 'beta',
  'GU9' : 'alpha',
  'GUF' : 'alpha',
  'GUP' : 'alpha',
  'GUZ' : 'beta',
  'GYP' : 'alpha',
  'H2P' : 'beta',
  'HSG' : 'alpha',
  'HSH' : 'beta',
  'HSJ' : 'beta',
  'HSQ' : 'alpha',
  'HSR' : 'beta',
  'HSU' : 'beta',
  'HSX' : 'alpha',
  'HSY' : 'alpha',
  'HSZ' : 'beta',
  'IDG' : 'beta',
  'IDR' : 'beta',
  'IDS' : 'beta',
  'IDT' : 'beta',
  'IDU' : 'alpha',
  'IDX' : 'beta',
  'IDY' : 'beta',
  'IN1' : 'beta',
  'IPT' : 'beta',
  'ISL' : 'alpha',
  'KBG' : 'beta',
  'KDM' : 'beta',
  'L6S' : 'alpha',
  'LDY' : 'alpha',
  'LGU' : 'beta',
  'LVZ' : 'alpha',
  'LXB' : 'beta',
  'LXZ' : 'alpha',
  'M6P' : 'alpha',
  'M8C' : 'beta',
  'MA1' : 'alpha',
  'MA2' : 'alpha',
  'MA3' : 'alpha',
  'MAG' : 'beta',
  'MAN' : 'alpha',
  'MAT' : 'alpha',
  'MAV' : 'beta',
  'MAW' : 'alpha',
  'MBG' : 'beta',
  'MCU' : 'alpha',
  'MDA' : 'beta',
  'MDP' : 'beta',
  'MFA' : 'alpha',
  'MFB' : 'beta',
  'MFU' : 'alpha',
  'MG5' : 'beta',
  'MGA' : 'beta',
  'MGL' : 'beta',
  'MMA' : 'alpha',
  'MRP' : 'alpha',
  'MXY' : 'beta',
  'N1L' : 'alpha',
  'NAA' : 'beta',
  'NAG' : 'beta',
  'NDG' : 'alpha',
  'NED' : 'beta',
  'NG1' : 'beta',
  'NG6' : 'beta',
  'NGA' : 'beta',
  'NGC' : 'beta',
  'NGE' : 'alpha',
  'NGL' : 'beta',
  'NGS' : 'beta',
  'NGY' : 'alpha',
  'NM6' : 'beta',
  'NM9' : 'beta',
  'OPM' : 'alpha',
  'ORP' : 'alpha',
  'P6P' : 'alpha',
  'PRP' : 'beta',
  'PSV' : 'alpha',
  'R1P' : 'beta',
  'RAA' : 'alpha',
  'RAE' : 'alpha',
  'RAM' : 'alpha',
  'RAO' : 'alpha',
  'RDP' : 'beta',
  'RER' : 'alpha',
  'RF5' : 'alpha',
  'RG1' : 'beta',
  'RGG' : 'beta',
  'RHA' : 'beta',
  'RIB' : 'alpha',
  'RIP' : 'beta',
  'RPA' : 'alpha',
  'RST' : 'alpha',
  'RUU' : 'alpha',
  'SG4' : 'alpha',
  'SG5' : 'alpha',
  'SG6' : 'alpha',
  'SG7' : 'alpha',
  'SGA' : 'beta',
  'SGC' : 'beta',
  'SGD' : 'alpha',
  'SGN' : 'alpha',
  'SHB' : 'alpha',
  'SHG' : 'beta',
  'SOE' : 'alpha',
  'SSG' : 'beta',
  'SUS' : 'alpha',
  'TM9' : 'beta',
  'TMR' : 'beta',
  'TMX' : 'beta',
  'TOA' : 'alpha',
  'TOC' : 'alpha',
  'TYV' : 'alpha',
  'UDC' : 'alpha',
  'X1X' : 'beta',
  'X2F' : 'alpha',
  'XLF' : 'beta',
  'XXR' : 'alpha',
  'XYP' : 'beta',
  'XYS' : 'alpha',
  'Z6J' : 'alpha',
}

if __name__=="__main__":
  for code in ["MAN",
               "BMA",
               "NAG",
               "FUL",
               "FUC",
              ]:
    print code, alpha_beta.get(code, None),volumes.get(code, None)
