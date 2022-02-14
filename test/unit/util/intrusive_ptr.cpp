#include <yaclib/config.hpp>
#include <yaclib/util/detail/atomic_counter.hpp>
#include <yaclib/util/helper.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/ref.hpp>

#include <cstddef>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;

class Core : public IRef {
 public:
  static std::size_t sInstances;

  Core(const Core&) = delete;
  Core& operator=(const Core&) = delete;

  Core() {
    ++sInstances;
  }

  ~Core() override {
    --sInstances;
  }
};

size_t Core::sInstances = 0;

class X : public Core {};
class Y : public X {};

using CounterX = detail::AtomicCounter<X>;
using CounterY = detail::AtomicCounter<Y>;

TEST(ctor, default) {
  IntrusivePtr<Core> px;
  EXPECT_EQ(px.Get(), nullptr);
  EXPECT_EQ(px, nullptr);
  EXPECT_EQ(nullptr, px);
}

TEST(ctor, pointer) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<Core> px{nullptr};
    EXPECT_EQ(px, nullptr);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<Core> px{NoRefTag{}, nullptr};
    EXPECT_EQ(px, nullptr);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    auto* p = new CounterX{1};
    EXPECT_EQ(p->GetRef(), 1);

    EXPECT_EQ(Core::sInstances, 1);

    IntrusivePtr<Core> px{NoRefTag{}, p};
    EXPECT_EQ(px.Get(), p);
    EXPECT_EQ(px, p);
    EXPECT_EQ(p, px);
    EXPECT_EQ(p->GetRef(), 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    auto* p = new CounterX{1};
    EXPECT_EQ(p->GetRef(), 1);

    p->IncRef();
    EXPECT_EQ(p->GetRef(), 2);

    IntrusivePtr<Core> pb{NoRefTag{}, p};
    {
      IntrusivePtr<CounterX> pc{NoRefTag{}, p};
      EXPECT_EQ(pb, pc);
      EXPECT_EQ(p->GetRef(), 2);
    }
    EXPECT_EQ(p->GetRef(), 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

TEST(ctor, copy) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<CounterX> pc1;
    IntrusivePtr<CounterX> pc2{pc1};
    EXPECT_EQ(pc1, pc2);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<Y> py;
    IntrusivePtr<X> px{py};
    EXPECT_EQ(py, px);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<X> px1 = MakeIntrusive<X>();
    IntrusivePtr<X> px2(px1);
    EXPECT_EQ(px1, px2);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<Y> py = MakeIntrusive<Y>();
    IntrusivePtr<X> px(py);
    EXPECT_EQ(py, px);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

TEST(dtor, simple) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<CounterX> pc1;
    IntrusivePtr<CounterX> pc2{pc1};
    EXPECT_EQ(pc1, pc2);
  }
  EXPECT_EQ(Core::sInstances, 0);

  {
    auto x = new CounterX{1};
    IntrusivePtr<X> px1{NoRefTag{}, x};
    EXPECT_EQ(x->GetRef(), 1);
    EXPECT_EQ(Core::sInstances, 1);
    {
      IntrusivePtr<X> px2{x};
      EXPECT_EQ(x->GetRef(), 2);
      EXPECT_EQ(Core::sInstances, 1);
    }
    EXPECT_EQ(x->GetRef(), 1);
    EXPECT_EQ(Core::sInstances, 1);
  }
  EXPECT_EQ(Core::sInstances, 0);
}

TEST(assign, copy) {
  EXPECT_EQ(Core::sInstances, 0);

  {
    IntrusivePtr<X> p1;
    p1 = p1;
    EXPECT_EQ(p1, p1);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    IntrusivePtr<X> p2;

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    IntrusivePtr<X> p3(p1);

    p1 = p3;

    EXPECT_EQ(p1, p3);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    IntrusivePtr<X> p4 = MakeIntrusive<X>();

    EXPECT_EQ(Core::sInstances, 1);

    p1 = p4;

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterX&>(*p1).GetRef(), 2);

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterX&>(*p4).GetRef(), 1);

    p4 = p3;

    EXPECT_EQ(p4, p3);
    EXPECT_EQ(Core::sInstances, 0);
  }
}

TEST(assign, conversation) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<X> p1;

    IntrusivePtr<Y> p2;

    p1 = p2;
    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    IntrusivePtr<Y> p4 = MakeIntrusive<Y>();

    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 1);

    IntrusivePtr<X> p5(p4);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p1 = p4;

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterY&>(*p1).GetRef(), 3);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 3);

    p1 = p2;

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p4 = p2;
    p5 = p2;

    EXPECT_EQ(p4, p2);
    EXPECT_EQ(Core::sInstances, 0);
  }
}

TEST(assign, pointer) {
  EXPECT_EQ(Core::sInstances, 0);
  {
    IntrusivePtr<X> p1;

    p1 = p1.Get();

    EXPECT_EQ(p1, p1);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    IntrusivePtr<X> p2;

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    IntrusivePtr<X> p3(p1);

    p1 = p3.Get();

    EXPECT_EQ(p1, p3);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    IntrusivePtr<X> p4 = MakeIntrusive<X>();

    EXPECT_EQ(Core::sInstances, 1);

    p1 = p4.Get();

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterX&>(*p1).GetRef(), 2);

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);

    p4 = p3.Get();

    EXPECT_EQ(p4, p3);
    EXPECT_EQ(Core::sInstances, 0);
  }

  {
    IntrusivePtr<X> p1;
    IntrusivePtr<Y> p2;

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_TRUE(p1 ? false : true);
    EXPECT_FALSE(p1);
    EXPECT_EQ(p1, nullptr);

    EXPECT_EQ(Core::sInstances, 0);

    IntrusivePtr<Y> p4 = MakeIntrusive<Y>();

    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 1);

    IntrusivePtr<X> p5(p4);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p1 = p4.Get();

    EXPECT_EQ(Core::sInstances, 1);

    EXPECT_EQ(p1, p4);

    EXPECT_EQ(static_cast<CounterY&>(*p1).GetRef(), 3);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 3);

    p1 = p2.Get();

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(Core::sInstances, 1);
    EXPECT_EQ(static_cast<CounterY&>(*p4).GetRef(), 2);

    p4 = p2.Get();
    p5 = p2.Get();

    EXPECT_EQ(p4, p2);
    EXPECT_EQ(Core::sInstances, 0);
  }
}
//
// TEST(reset, simple) {
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    IntrusivePtr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset();
//    BOOST_TEST(px.get() == 0);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    px.reset();
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    IntrusivePtr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    IntrusivePtr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0, false);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    IntrusivePtr<X> px(new X);
//    BOOST_TEST(N::base::instances == 1);
//
//    px.reset(0, true);
//    BOOST_TEST(px.get() == 0);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    IntrusivePtr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset(p, true);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    intrusive_ptr_add_ref(p);
//    BOOST_TEST(p->use_count() == 1);
//
//    IntrusivePtr<X> px;
//    BOOST_TEST(px.get() == 0);
//
//    px.reset(p, false);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    IntrusivePtr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    IntrusivePtr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p, true);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//
//  {
//    IntrusivePtr<X> px(new X);
//    BOOST_TEST(px.get() != 0);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    intrusive_ptr_add_ref(p);
//    BOOST_TEST(p->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 2);
//
//    px.reset(p, false);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    BOOST_TEST(N::base::instances == 1);
//  }
//
//  BOOST_TEST(N::base::instances == 0);
//}
//
// TEST(access, simple) {
//  {
//    IntrusivePtr<X> px;
//    BOOST_TEST(px ? false : true);
//    BOOST_TEST(!px);
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    IntrusivePtr<X> px(0);
//    BOOST_TEST(px ? false : true);
//    BOOST_TEST(!px);
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    IntrusivePtr<X> px(new X);
//    BOOST_TEST(px ? true : false);
//    BOOST_TEST(!!px);
//    BOOST_TEST(&*px == px.get());
//    BOOST_TEST(px.operator->() == px.get());
//
//    BOOST_TEST(get_pointer(px) == px.get());
//  }
//
//  {
//    IntrusivePtr<X> px;
//    X* detached = px.detach();
//    BOOST_TEST(px.get() == 0);
//    BOOST_TEST(detached == 0);
//  }
//
//  {
//    X* p = new X;
//    BOOST_TEST(p->use_count() == 0);
//
//    IntrusivePtr<X> px(p);
//    BOOST_TEST(px.get() == p);
//    BOOST_TEST(px->use_count() == 1);
//
//    X* detached = px.detach();
//    BOOST_TEST(px.get() == 0);
//
//    BOOST_TEST(detached == p);
//    BOOST_TEST(detached->use_count() == 1);
//
//    delete detached;
//  }
//}

// TEST(swap, simple) {
//   {
//     IntrusivePtr<X> px;
//     IntrusivePtr<X> px2;
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == 0);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   {
//     X* p = new X;
//     IntrusivePtr<X> px;
//     IntrusivePtr<X> px2(p);
//     IntrusivePtr<X> px3(px2);
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == p);
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(px2.get() == 0);
//     BOOST_TEST(px3.get() == p);
//     BOOST_TEST(px3->use_count() == 2);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == 0);
//     BOOST_TEST(px2.get() == p);
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px3.get() == p);
//     BOOST_TEST(px3->use_count() == 2);
//   }
//
//   {
//     X* p1 = new X;
//     X* p2 = new X;
//     IntrusivePtr<X> px(p1);
//     IntrusivePtr<X> px2(p2);
//     IntrusivePtr<X> px3(px2);
//
//     px.swap(px2);
//
//     BOOST_TEST(px.get() == p2);
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(px2.get() == p1);
//     BOOST_TEST(px2->use_count() == 1);
//     BOOST_TEST(px3.get() == p2);
//     BOOST_TEST(px3->use_count() == 2);
//
//     using std::swap;
//     swap(px, px2);
//
//     BOOST_TEST(px.get() == p1);
//     BOOST_TEST(px->use_count() == 1);
//     BOOST_TEST(px2.get() == p2);
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px3.get() == p2);
//     BOOST_TEST(px3->use_count() == 2);
//   }
// }
//
// namespace n_comparison {
//
// template <class T, class U>
// void test2(IntrusivePtr<T> const& p, IntrusivePtr<U> const&
// q) {
//   BOOST_TEST((p == q) == (p.get() == q.get()));
//   BOOST_TEST((p != q) == (p.get() != q.get()));
// }
//
// template <class T>
// void test3(IntrusivePtr<T> const& p, IntrusivePtr<T> const&
// q) {
//   BOOST_TEST((p == q) == (p.get() == q.get()));
//   BOOST_TEST((p.get() == q) == (p.get() == q.get()));
//   BOOST_TEST((p == q.get()) == (p.get() == q.get()));
//   BOOST_TEST((p != q) == (p.get() != q.get()));
//   BOOST_TEST((p.get() != q) == (p.get() != q.get()));
//   BOOST_TEST((p != q.get()) == (p.get() != q.get()));
//
//   // 'less' moved here as a g++ 2.9x parse error workaround
//   std::less<T*> less;
//   BOOST_TEST((p < q) == less(p.get(), q.get()));
// }
//
// void test() {
//   {
//     IntrusivePtr<X> px;
//     test3(px, px);
//
//     IntrusivePtr<X> px2;
//     test3(px, px2);
//
//     IntrusivePtr<X> px3(px);
//     test3(px3, px3);
//     test3(px, px3);
//   }
//
//   {
//     IntrusivePtr<X> px;
//
//     IntrusivePtr<X> px2(new X);
//     test3(px, px2);
//     test3(px2, px2);
//
//     IntrusivePtr<X> px3(new X);
//     test3(px2, px3);
//
//     IntrusivePtr<X> px4(px2);
//     test3(px2, px4);
//     test3(px4, px4);
//   }
//
//   {
//     IntrusivePtr<X> px(new X);
//
//     IntrusivePtr<Y> py(new Y);
//     test2(px, py);
//
//     IntrusivePtr<X> px2(py);
//     test2(px2, py);
//     test3(px, px2);
//     test3(px2, px2);
//   }
// }
//
// }  // namespace n_comparison
//
// namespace n_static_cast {
//
// void test() {
//   {
//     IntrusivePtr<X> px(new Y);
//
//     IntrusivePtr<Y> py = boost::static_pointer_cast<Y>(px);
//     BOOST_TEST(px.get() == py.get());
//     BOOST_TEST(px->use_count() == 2);
//     BOOST_TEST(py->use_count() == 2);
//
//     IntrusivePtr<X> px2(py);
//     BOOST_TEST(px2.get() == px.get());
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     IntrusivePtr<Y> py =
//         boost::static_pointer_cast<Y>(IntrusivePtr<X>(new Y));
//     BOOST_TEST(py.get() != 0);
//     BOOST_TEST(py->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_static_cast
//
// namespace n_const_cast {
//
// void test() {
//   {
//     IntrusivePtr<X const> px;
//
//     IntrusivePtr<X> px2 = boost::const_pointer_cast<X>(px);
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   {
//     IntrusivePtr<X> px2 =
//         boost::const_pointer_cast<X>(IntrusivePtr<X const>());
//     BOOST_TEST(px2.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     IntrusivePtr<X const> px(new X);
//
//     IntrusivePtr<X> px2 = boost::const_pointer_cast<X>(px);
//     BOOST_TEST(px2.get() == px.get());
//     BOOST_TEST(px2->use_count() == 2);
//     BOOST_TEST(px->use_count() == 2);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     IntrusivePtr<X> px =
//         boost::const_pointer_cast<X>(IntrusivePtr<X const>(new X));
//     BOOST_TEST(px.get() != 0);
//     BOOST_TEST(px->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_const_cast
//
// namespace n_dynamic_cast {
//
// void test() {
//   {
//     IntrusivePtr<X> px;
//
//     IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     IntrusivePtr<Y> py =
//         boost::dynamic_pointer_cast<Y>(IntrusivePtr<X>());
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     IntrusivePtr<X> px(static_cast<X*>(0));
//
//     IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(
//         IntrusivePtr<X>(static_cast<X*>(0)));
//     BOOST_TEST(py.get() == 0);
//   }
//
//   {
//     IntrusivePtr<X> px(new X);
//
//     IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     IntrusivePtr<Y> py =
//         boost::dynamic_pointer_cast<Y>(IntrusivePtr<X>(new X));
//     BOOST_TEST(py.get() == 0);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     IntrusivePtr<X> px(new Y);
//
//     IntrusivePtr<Y> py = boost::dynamic_pointer_cast<Y>(px);
//     BOOST_TEST(py.get() == px.get());
//     BOOST_TEST(py->use_count() == 2);
//     BOOST_TEST(px->use_count() == 2);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
//
//   {
//     IntrusivePtr<X> px(new Y);
//
//     IntrusivePtr<Y> py =
//         boost::dynamic_pointer_cast<Y>(IntrusivePtr<X>(new Y));
//     BOOST_TEST(py.get() != 0);
//     BOOST_TEST(py->use_count() == 1);
//   }
//
//   BOOST_TEST(N::base::instances == 0);
// }
//
// }  // namespace n_dynamic_cast
//
// namespace n_transitive {
//
// struct X : public N::base {
//   IntrusivePtr<X> next;
// };
//
// void test() {
//   IntrusivePtr<X> p(new X);
//   p->next = IntrusivePtr<X>(new X);
//   BOOST_TEST(!p->next->next);
//   p = p->next;
//   BOOST_TEST(!p->next);
// }
//
// }  // namespace n_transitive
//
// namespace n_report_1 {
//
// class foo : public N::base {
//  public:
//   foo() : m_self(this) {
//   }
//
//   void suicide() {
//     m_self = 0;
//   }
//
//  private:
//   IntrusivePtr<foo> m_self;
// };
//
// void test() {
//   foo* foo_ptr = new foo;
//   foo_ptr->suicide();
// }
//
// }  // namespace n_report_1

}  // namespace
