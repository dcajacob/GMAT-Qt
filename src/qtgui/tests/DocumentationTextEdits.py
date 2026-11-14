"""Apply nonoverlapping source edits without changing their original offsets."""
def apply_edits(original,edits):
 ordered=sorted(edits,key=lambda e:(e[0],e[1]));parts=[];cursor=0;previous=None
 for start,end,replacement in ordered:
  if not isinstance(start,int) or not isinstance(end,int) or start<0 or end<start or end>len(original):raise ValueError('Invalid documentation edit span')
  if previous is not None and (start<cursor or (start,end)==previous):raise ValueError('Overlapping/duplicate documentation edits')
  parts.extend([original[cursor:start],replacement]);cursor=end;previous=(start,end)
 parts.append(original[cursor:]);return ''.join(parts)
