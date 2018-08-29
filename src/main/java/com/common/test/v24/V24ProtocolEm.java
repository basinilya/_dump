package com.common.test.v24;

public enum V24ProtocolEm {
   ISN(0),
   ALARM(1),
   ALPHA(2),
   ALPHAX(3),
   ESPA(4),
   FPA(5);

   private final int id;

   private V24ProtocolEm(int id) {
      this.id = id;// 23
   }// 24

   public int getId() {
      return this.id;// 27
   }
}
